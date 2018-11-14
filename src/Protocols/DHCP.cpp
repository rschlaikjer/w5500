#include <W5500/Protocols/DHCP.hpp>

#include <stdio.h>

namespace W5500 {
    namespace Protocols {
        namespace DHCP {

    void Client::update() {
        switch (_state) {
            case State::START:
                fsm_start();
                break;
            case State::DISCOVER:
                fsm_discover();
                break;
            case State::REQUEST:
                break;
            case State::LEASED:
                break;
            case State::RENEW:
                break;
            case State::RELEASE:
                break;
        }
    }

    void Client::reset_current_lease() {
        // Clear any lease state
        memset(_local_ip, 0, sizeof(_local_ip));
        memset(_dhcp_server_ip, 0, sizeof(_dhcp_server_ip));
        memset(_subnet_mask, 0, sizeof(_subnet_mask));
        memset(_gateway_ip, 0, sizeof(_gateway_ip));
        memset(_dns_server_ip, 0, sizeof(_dns_server_ip));
    }

    void Client::fsm_start() {
        _driver.bus().log("Requesting DHCP lease\n");

        // Try and open a UDP socket
        if (_socket == -1) {
            _socket = _driver.open_socket(SocketMode::UDP);
            if (_socket == -1) {
                _driver.bus().log("Failed to open socket for DHCP client!\n");
                return;
            }
            _driver.bus().log("Opened UDP socket %d\n", _socket);

            // Configure the socket
            uint8_t broadcast[4] = {255, 255, 255, 255};
            _driver.set_socket_dest_ip_address(_socket, broadcast);
            _driver.set_socket_dest_port(_socket, dhcp_server_port);
            _driver.set_socket_src_port(_socket, dhcp_client_port);
            _driver.send_socket_command(_socket, Registers::Socket::CommandValue::CONNECT);
        }

        // Generate a transaction ID
        _initial_xid = _driver.bus().random();
        _xid = _initial_xid;
        _driver.bus().log("Initial xid: %x\n", _xid);

        // Store the start time
        _lease_request_start = _driver.bus().millis();

        _xid++;
        send_dhcp_packet(DhcpMessageType::DISCOVER);
        _last_discover_broadcast = _driver.bus().millis();
        _state = State::DISCOVER;
    }

    void Client::fsm_discover() {
        // Get the int flags for this socket
        auto flags = _driver.get_socket_interrupt_flags(_socket);

        // Check if we received data
        if (flags & Registers::Socket::InterruptFlags::RECV) {
            // Clear data received interrupt flag
            _driver.clear_socket_iterrupt_flag(
                _socket, Registers::Socket::InterruptFlags::RECV);

            // Read the pending data
            parse_dhcp_response();
        }

        // If it's been long enough, send another discover
        if (_driver.bus().millis() - _last_discover_broadcast
                > discover_broadcast_interval_ms) {
            _xid++;
            send_dhcp_packet(DhcpMessageType::DISCOVER);
            _last_discover_broadcast = _driver.bus().millis();
        }
    }

    void Client::parse_dhcp_response() {
        // Get the number of bytes received
        uint16_t bytes_received = _driver.get_rx_byte_count(_socket);
        _driver.bus().log("Received %u bytes DHCP data\n", bytes_received);

        // Read the fixed-size header
        uint8_t data[ParseContext::total_size];
        _driver.read(_socket, data, ParseContext::total_size);

        // Parse the fixed header
        ParseContext parsed;
        size_t consumed = parsed.consume(data, ParseContext::total_size);

        // If the op is not BOOTREPLY, ignore the data.
        if (parsed.op != 1) { // DHCP_BOOTREPLY) {
            _driver.bus().log("Op is %u not BOOTREPLY< ignoring\n");
            return;
        }

        // If chaddr != our own MAC, ignore
        uint8_t mac[6];
        _driver.get_mac(mac);
        if (memcmp(parsed.chaddr, mac, 6) != 0) {
            _driver.bus().log("Mismatched MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                parsed.chaddr[0], parsed.chaddr[1], parsed.chaddr[2],
                parsed.chaddr[3], parsed.chaddr[4], parsed.chaddr[5]);
            return;
        }

        // If the transaction ID is out of range, ingore
        if (parsed.xid < _initial_xid || parsed.xid > _xid) {
            _driver.bus().log("XID %u is out of range %u -> %u\n",
                parsed.xid, _initial_xid, parsed.xid);
            return;
        }

        // Copy any offered address to our local IP
        _driver.bus().log("Got DHCPOFFER: %u.%u.%u.%u\n",
            parsed.yiaddr[0], parsed.yiaddr[1], parsed.yiaddr[2], parsed.yiaddr[3]);
        memcpy(_local_ip, parsed.yiaddr, 4);

        // Skip to the option bytes - 206 bytes
        size_t skipped = _driver.read(_socket, nullptr, 206);
        if (skipped != 206) {
            _driver.bus().log("Failed to skip to option bytes\n");
            return;
        }

        uint8_t opt_len = 0;
        uint8_t type = 0;

        uint16_t avail = _driver.get_rx_byte_count(_socket);
        uint8_t d[avail];
        _driver.peek(_socket, d, avail);
        for (uint16_t i = 0; i < avail; i++) {
            if (i % 16 == 0)
                printf("\n");
            printf("%3u ", d[i]);
        }

        // Update the remaining byte count
        while (_driver.get_rx_byte_count(_socket) > 0) {
            DhcpOption opt = DhcpOption(_driver.read(_socket));
            switch (opt) {
                case DhcpOption::END_OPTIONS:
                    _driver.bus().log("Done parsing options\n");
                    break;
                case DhcpOption::MESSAGE_TYPE:
                    opt_len = _driver.read(_socket);
                    type = _driver.read(_socket);
                    _driver.bus().log("Message type: %u\n", type);
                    break;
                case DhcpOption::SUBNET_MASK:
                    opt_len = _driver.read(_socket);
                    _driver.read(_socket, _subnet_mask, 4);
                    _driver.bus().log("Subnet mask: %u.%u.%u.%u\n",
                        _subnet_mask[0], _subnet_mask[1],
                        _subnet_mask[2], _subnet_mask[3]);
                    break;
                case DhcpOption::ROUTERS_ON_SUBNET:
                    opt_len = _driver.read(_socket);
                    _driver.read(_socket, _gateway_ip, 4);
                    _driver.bus().log("Gateway: %u.%u.%u.%u\n",
                        _gateway_ip[0], _gateway_ip[1],
                        _gateway_ip[2], _gateway_ip[3]);
                    // Skip any extra routers
                    _driver.read(_socket, nullptr, opt_len - 4);
                    break;
                case DhcpOption::DNS:
                    opt_len = _driver.read(_socket);
                    _driver.read(_socket, _dns_server_ip, 4);
                    _driver.bus().log("DNS: %u.%u.%u.%u\n",
                        _dns_server_ip[0], _dns_server_ip[1],
                        _dns_server_ip[2], _dns_server_ip[3]);
                    // Skip any extra hosts
                    _driver.read(_socket, nullptr, opt_len - 4);
                    break;
                default:
                    // Just skip these
                    opt_len = _driver.read(_socket);
                    _driver.bus().log("Skipping unknown opt %u of len %u\n",
                        opt, opt_len);
                    for (uint8_t i = 0; i < opt_len; i++)
                        printf("0x%02x ", _driver.read(_socket));
                    uint8_t f;
                    _driver.peek(_socket, &f, 1);
                    printf("Peek: %u\n", f);
                    // _driver.read(_socket, nullptr, opt_len);
            }
        }
    }

    uint16_t Client::seconds_elapsed() {
        auto delta_ms = _lease_request_start - _driver.bus().millis();
        return delta_ms / 1000;
    }

    void Client::send_dhcp_packet(DhcpMessageType type) {
        // Buffer for batching data to W5500 IC
        uint8_t buffer[32];
        memset(buffer, 0x00, sizeof(buffer));

        // Constant flags
        buffer[0] = 0x01; // Operation
        buffer[1] = 0x01; // HTYPE
        buffer[2] = 0x06; // HLEN
        buffer[3] = 0x00; // HOPS

        // Transaction ID
        // _xid = random();
        embed_u32(&buffer[4], _xid);

        // Seconds elapsed
        embed_u16(&buffer[8], seconds_elapsed());

        // DHCP flags
        embed_u16(&buffer[10], 0x8000); // broadcast

        // ciaddr, yiaddr, siaddr, giaddr are memset 0

        // Copy data to W5500
        _driver.write(_socket, buffer, 28);

        // Re-clear buffer
        memset(buffer, 0x00, sizeof(buffer));

        // Set MAC, flush. Total size of CHADDR field is 16.
        _driver.get_mac(buffer);
        _driver.write(_socket, buffer, 16);

        // Zero mac out again
        memset(buffer, 0x00, 6);

        // Write zeros for sname (64b) and file (128b)
        for(int i = 0; i < 6; i++) {
            _driver.write(_socket, buffer, 32);
        }

        // Set magic cookie
        embed_u32(buffer, magic_cookie);

        // Set message type
        buffer[4] = static_cast<uint8_t>(DhcpOption::MESSAGE_TYPE);
        buffer[5] = 0x01; // 1 byte
        buffer[6] = static_cast<uint8_t>(type);

        // Client ID (MAC)
        buffer[7] = static_cast<uint8_t>(DhcpOption::CLIENT_IDENTIFIER);
        buffer[8] = 0x07;
        buffer[9] = 0x01;
        _driver.get_mac(&buffer[10]);

        // Host name
        buffer[16] = 53; // Option: Host name
        buffer[0] = static_cast<uint8_t>(DhcpOption::CLIENT_HOSTNAME);
        uint8_t hostname_len = strlen(_hostname);
        buffer[17] = hostname_len;
        // Write buffer
        _driver.write(_socket, buffer, 18);
        // Write the host name
        _driver.write(_socket, reinterpret_cast<const uint8_t*>(_hostname), hostname_len);

        // DHCP request message needs to include the requested IP & DHCP server
        if (type == DhcpMessageType::REQUEST) {
            // Set requested IP
            buffer[0] = static_cast<uint8_t>(DhcpOption::REQUESTED_IP_ADDR);
            buffer[1] = 0x04; // IPs are 4 bytes
            memcpy(&buffer[2], _local_ip, 4);

            // Set target server
            buffer[6] = static_cast<uint8_t>(DhcpOption::SERVER_IDENTIFIER);
            buffer[7] = 0x04; // IPs are 4 bytes
            memcpy(&buffer[8], _dhcp_server_ip, 4);

            _driver.write(_socket, buffer, 12);
        }

        // Parameter request
        buffer[0] = static_cast<uint8_t>(DhcpOption::PARAM_REQUEST);
        buffer[1] = 0x06; // Request 6 params
        buffer[2] = static_cast<uint8_t>(DhcpOption::SUBNET_MASK);
        buffer[3] = static_cast<uint8_t>(DhcpOption::ROUTERS_ON_SUBNET);
        buffer[4] = static_cast<uint8_t>(DhcpOption::DNS);
        buffer[5] = static_cast<uint8_t>(DhcpOption::DOMAIN_NAME);
        buffer[6] = static_cast<uint8_t>(DhcpOption::DHCP_T1_VALUE);
        buffer[7] = static_cast<uint8_t>(DhcpOption::DHCP_T2_VALUE);
        buffer[8] = static_cast<uint8_t>(DhcpOption::END_OPTIONS);
        _driver.write(_socket, buffer, 9);

        // Trigger a send of the buffered command
        _driver.send(_socket);
    }

    void Client::embed_u16(uint8_t *buffer, uint16_t value) {
        buffer[0] = (value >>  8) & 0xFF;
        buffer[1] =  value        & 0xFF;
    }

    void Client::embed_u32(uint8_t *buffer, uint32_t value) {
        buffer[0] = (value >> 24) & 0xFF;
        buffer[1] = (value >> 16) & 0xFF;
        buffer[2] = (value >>  8) & 0xFF;
        buffer[3] =  value        & 0xFF;
    }

}}}
