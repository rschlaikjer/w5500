#include <W5500/Protocols/DNS.hpp>

#include <stdio.h>

namespace W5500 {
    namespace Protocols {
        namespace DNS {

            namespace {
                const int max_packet_size = 256;
            }

    void Client::update() {
        if (!_socket.ready()) {
            if (!_socket.init()) {
                _driver.bus().log("Failed to open socket for DHCP client!\n");
                return;
            }

            // Configure the socket
            _socket.set_dest_ip(_ip[0], _ip[1], _ip[2], _ip[3]);
            _socket.set_dest_port(port);
            _socket.set_source_port(port);
        }

        // Get the int flags for this socket
        auto flags = _socket.get_interrupt_flags();

        // Check for packet rx
        if (flags & Registers::Socket::InterruptFlags::RECV) {
            // Clear data received interrupt flag
            _socket.clear_interrupt_flag(Registers::Socket::InterruptFlags::RECV);

            // Try and parse the response
            while (parse_packet());
        }
    }

    bool Client::parse_packet() {
        uint8_t source_ip[4];
        uint16_t source_port;
        const int packet_size = _socket.read_packet_header(source_ip, source_port);
        if (packet_size < 0) {
            return false;
        }

        if (packet_size > max_packet_size) {
            // Throw it away
            _socket.read(nullptr, packet_size);
            return true;
        }

        // Read the packet data
        uint8_t buffer[max_packet_size];
        _socket.read(buffer, packet_size);

        // Check that this is actually an answer
        uint16_t query_id = decode_short(&buffer[0]);
        bool is_answer = buffer[2] & (1 << 7);
        // If not, just ignore this packet and loop.
        if (!is_answer) {
            return true;
        }
        // bool is_truncated = buffer[2] & (1 << 1);

        // If the response code is non-zero, this response is an error
        uint8_t rcode = buffer[3] & 0b1111;
        if (rcode != 0) {
            _driver.bus().log("DNS resolution error for request %u: rcode %u\n",
                query_id, rcode);
            return true;
        }

        const uint16_t query_count = decode_short(&buffer[4]);
        // Number of entries in the answer section. Zero here.
        const uint16_t answer_count = decode_short(&buffer[6]);
        // Number of name servers in the Authority section. Zero here.
        // const uint16_t authority_count = decode_short(&buffer[8]);
        // Number of additional resource records. Zero here.
        // const uint16_t additional_count = decode_short(&buffer[10]);

        // First, we need to skip over all the queries.
        size_t offset = 12;
        for (uint16_t i = 0; i < query_count; i++) {
            // Don't bother to parse back the domain name, just skip over it
            // by jumping to all the octet size values until we get to the terminator.
            while (buffer[offset] > 0) {
                if (buffer[offset] & 0b11000000) {
                    // This is a compressed value - if we were parsing, we'd read
                    // the label + data from this field interpreted as a 16-bit int
                    // Instead, we'll just skip these two bytes.
                    offset += 2;
                } else {
                    offset += buffer[offset] + 1;
                }
            }

            // Once we've read past the name, skip 4 more bytes for type & class
            // const uint16_t query_type = decode_short(&buffer[offset+1]);
            // const uint16_t query_class = decode_short(&buffer[offset+3]);

            // Increment offset for next loop
            offset += 4 + 1;
        }

        // Now, we can parse all the answers
        for (uint16_t i = 0; i < answer_count; i++) {
            // Again, skip all the names
            while (buffer[offset] > 0) {
                if (buffer[offset] & 0b11000000) {
                    // This is a compressed value - normally need to interpret
                    // as 16-bit int, mask off the first two bits, and continue
                    // reading from there. We should be safe to assume that this
                    // jump never returns, and so this terminates the string.
                    // Only advance 1, since the following code expects offset to be
                    // on the final byte in the label, not the next byte after
                    offset += 1;
                    break;
                } else {
                    offset += buffer[offset] + 1;
                }
            }

            // Offset is now the index of the final '\0' in the domain name.
            // Advancing by one should get us the first byte of the 'type' field
            const uint16_t response_type = decode_short(&buffer[offset+1]);

            const uint16_t response_class = decode_short(&buffer[offset+3]);

            // If this is an A answer with an IP address, cache it
            if (response_type == 0x01 && response_class == 0x01) {
                const uint32_t ttl_secs = decode_int(&buffer[offset+5]);
                // const uint16_t rdlength = decode_short(&buffer[offset+9]);

                // Cache this response
                const auto now_ms = _driver.bus().millis();
                _cache.store(
                    query_id, &buffer[offset+11],
                    now_ms, now_ms + ttl_secs * 1000
                );

                // For now, one address is good enough
                return true;
            }

            // If this answer wasn't satisfactory, keep looping and hope we get
            // a good one.
            offset += 15;
        }

        return true;
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

    void Client::encode_short(uint8_t *buf, uint16_t v) {
        buf[0] = (v >> 8) & 0xFF;
        buf[1] = v & 0xFF;
    }

    uint16_t Client::decode_short(uint8_t *buf) {
        return (buf[0] << 8) | buf[1];
    }

    uint32_t Client::decode_int(uint8_t *buf) {
        return (
            (((uint32_t)buf[0]) << 24) |
            (((uint32_t)buf[1]) << 16) |
            (((uint32_t)buf[2]) <<  8) |
             ((uint32_t)buf[3])
        );
    }

    bool Client::get(uint16_t query_id, uint8_t ip[4]) {
        return _cache.get(_driver.bus().millis(), query_id, ip);
    }

    bool Client::query(const char *hostname, uint16_t *query_id_out) {

        // Use the CRC16 of the hostname as a query ID, so that we can use it as
        // a stable cache reference
        const uint16_t query_id = Utility::CRC16::of(
            reinterpret_cast<const uint8_t*>(hostname), strlen(hostname));

        // Check if we already have this domain resolved in the cache, and if
        // so do nothing.
        if (_cache.has_entry(_driver.bus().millis(), query_id)) {
            *query_id_out = query_id;
            return true;
        }

        uint8_t buffer[32];
        encode_short(&buffer[0], query_id);
        buffer[2] = (
            (0 << 7) | // QR: 0 for query, 1 for response
            (0b0000 << 3) | // OPCODE: 0 for standard query
            (0 << 2) | // AA: Authoritative Answer: for responses only
            (0 << 1) | // TC: Truncated. For resp only.
            (1 << 0) // RD: Recursion Desired.
        );
        buffer[3] = (
            (0 << 7) | // RA: We do not personaly support recursion
            (0b000 << 4) | // Z: for future use.
            (0b0000) // RCODE: for responses only.
        );
        // Number of elements in the question section
        encode_short(&buffer[4], 1);
        // Number of entries in the answer section. Zero here.
        encode_short(&buffer[6], 0);
        // Number of name servers in the Authority section. Zero here.
        encode_short(&buffer[8], 0);
        // Number of additional resource records. Zero here.
        encode_short(&buffer[10], 0);

        // Write what we have to the buffer thusfar
        _socket.write(buffer, 12);

        // Now, we need to write our hostname, but in DNS format -
        // length-delimited series of octets for the domain and terminated
        // by a zero-length octet label.
        // e.g, www.google.com -> 0x3, 'www', 0x5, 'google', 0x3, 'com', 0x0
        size_t label_start = 0;
        for (size_t i = 0; ; i++) {
            // If we hit a period or end of string, write out the octet label
            if (hostname[i] == '.' || hostname[i] == '\0') {
                // The number of octets since the last label start
                const size_t octets_in_label = i - label_start;
                if (octets_in_label > 255) {
                    _driver.bus().log(
                        "DNS query error: hostname label exceeds 255 byte limit: %s",
                        hostname
                    );
                    _socket.close();
                    return false;
                }

                // Write the number of octets + the label to the chip buffer
                buffer[0] = octets_in_label;
                _socket.write(buffer, 1);
                _socket.write(
                    reinterpret_cast<const uint8_t*>(&hostname[label_start]),
                    octets_in_label
                );

                // If the character we hit was end-of-string NUL, break
                if (hostname[i] == '\0') {
                    break;
                }

                // Advance the label start pointer, skipping this period
                label_start = i + 1;
            }
        }

        // Finalize the hostname with a \0
        buffer[0] = 0x00;

        // Qtype is IPV4 A, 0x01
        encode_short(&buffer[1], 0x01);

        // Qclass is IP addresses, 0x01
        encode_short(&buffer[3], 0x01);

        // Copy to IC and send
        _socket.write(buffer, 5);
        _socket.send();

        // Set the output query ID
        *query_id_out = query_id;

        // Success
        return true;
    }

}}}
