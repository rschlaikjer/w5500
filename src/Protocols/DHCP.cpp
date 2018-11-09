#include <W5500/Protocols/DHCP.hpp>

namespace W5500 {
    namespace Protocols {
        namespace DHCP {

    void Client::discover() {

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

        _driver.end_packet(_socket);
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
