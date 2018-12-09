#include <W5500/Protocols/NTP.hpp>

#include <stdio.h>

namespace W5500 {
    namespace Protocols {
        namespace NTP {

    bool Client::update(uint64_t *current_time_ms) {
        if (!_socket.ready()) {
            if (!_socket.init()) {
                _driver.bus().log("Failed to open socket for NTP client!\n");
                return false;
            }

            // Configure the socket
            _socket.set_dest_ip(_ip[0], _ip[1], _ip[2], _ip[3]);
            _socket.set_dest_port(ntp_port);
            _socket.set_source_port(ntp_port);
        }

        // Get the int flags for this socket
        auto flags = _socket.get_interrupt_flags();

        // Check for packet rx
        bool parsed_valid_packet = false;
        if (flags & Registers::Socket::InterruptFlags::RECV) {
            // Clear data received interrupt flag
            _socket.clear_interrupt_flag(Registers::Socket::InterruptFlags::RECV);

            // Try and parse the response
            while (parse_packet(current_time_ms)) {
                parsed_valid_packet = true;
            }
        }

        // Check if it's been long enough since the last lock that we should poll
        const uint64_t time_since_last_response = _driver.bus().millis() - _last_ntp_response;
        const uint64_t poll_interval_ms = (1 << _poll_interval) * 1000;
        if (time_since_last_response > poll_interval_ms) {
            // Ensure we don't send requests too fast
            const uint64_t time_since_last_request = _driver.bus().millis() - _last_ntp_request;
            if (time_since_last_request > request_interval_ms) {
                send_request();
            }
        }

        // If we parsed any valid NTP frames, then the current_time_ms value
        // has been updated.
        return parsed_valid_packet;
    }

    void Client::set_server_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        _ip[0] = a;
        _ip[1] = b;
        _ip[2] = c;
        _ip[3] = d;
        if (_socket.ready()) {
            _socket.set_dest_ip(_ip[0], _ip[1], _ip[2], _ip[3]);
        }
    }

    bool Client::parse_packet(uint64_t *current_time_ms) {
        uint8_t source_ip[4];
        uint16_t source_port;
        const int packet_size = _socket.read_packet_header(source_ip, source_port);
        if (packet_size < 0) {
            return false;
        }

        if (packet_size > ntp_packet_size) {
            // Throw it away
            _socket.read(nullptr, packet_size);
            return true;
        }

        // Read the NTP packet data
        uint8_t buffer[ntp_packet_size];
        _socket.read(buffer, ntp_packet_size);

        // Get the origin timestamp
        uint32_t originate_timestamp_seconds = (
            (((uint32_t) buffer[40]) << 24) |
            (((uint32_t) buffer[41]) << 16) |
            (((uint32_t) buffer[42]) <<  8) |
             ((uint32_t) buffer[43])
        );

        // To convert to UNIX, subtract 70 years
        uint32_t unix_ts = originate_timestamp_seconds - seventy_years;

        // Get the seconds fraction
        uint32_t originate_timestamp_seconds_fraction = (
            (((uint32_t) buffer[44]) << 24) |
            (((uint32_t) buffer[45]) << 16) |
            (((uint32_t) buffer[46]) <<  8) |
             ((uint32_t) buffer[47])
        );

        // Convert fraction to ms and add to seconds value
        const uint32_t milliseconds = originate_timestamp_seconds_fraction / (0xFFFFFFFF / 1000);
        const uint64_t unix_millis = ((uint64_t) unix_ts * 1000) + milliseconds;

        // Set the output time value
        *current_time_ms = unix_millis;

        // Timekeeping
        _last_ntp_response = _driver.bus().millis();
        _poll_interval = buffer[2];

        // Receipt success
        return true;
    }

    void Client::send_request() {
        // Packet buffer
        uint8_t buffer[ntp_packet_size];
        memset(buffer, 0x00, sizeof(buffer));

        // We are using unicast operation, mode 3
        buffer[0] = (
            (0b100 << 3) | // SNTP version 4
            static_cast<uint8_t>(Mode::CLIENT) // Mode = client
        );

        // That's it! Send request.
        _socket.send(buffer, sizeof(buffer));

        // Update last request timestamp
        _last_ntp_request = _driver.bus().millis();
    }

}}}
