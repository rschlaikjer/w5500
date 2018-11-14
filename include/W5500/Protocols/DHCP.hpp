#ifndef _W5500__W5500_PROTOCOLS_DHCP_H_
#define _W5500__W5500_PROTOCOLS_DHCP_H_

#include <stdint.h>

#include <W5500/W5500.hpp>

namespace W5500 {
    namespace Protocols {
        namespace DHCP {

    // DHCP ports
    static const uint16_t dhcp_server_port = 67;
    static const uint16_t dhcp_client_port = 68;

    static const uint64_t discover_broadcast_interval_ms = 1000;

    // Magic cookie for DHCP requests
    static const uint32_t magic_cookie = 0x63825363;

    enum class State {
        START,
        DISCOVER,
        REQUEST,
        LEASED,
        RENEW,
        RELEASE
    };

    enum class DhcpOperation : uint8_t {
        REQUEST = 1,
        REPLY = 2
    };

    enum class DhcpMessageType : uint8_t {
        DISCOVER = 1,
        OFFER = 2,
        REQUEST = 3,
        DECLINE = 4,
        ACK = 5,
        NAK = 6,
        RELEASE = 7,
        INFORM = 8
    };

    enum class DhcpOption : uint8_t {
        SUBNET_MASK = 1,
        ROUTERS_ON_SUBNET = 3,
        DNS = 6,
        CLIENT_HOSTNAME = 12,
        DOMAIN_NAME = 15,
        REQUESTED_IP_ADDR = 50,
        MESSAGE_TYPE = 53,
        SERVER_IDENTIFIER = 54,
        PARAM_REQUEST = 55,
        DHCP_T1_VALUE = 58,
        DHCP_T2_VALUE = 59,
        CLIENT_IDENTIFIER = 61,
        END_OPTIONS = 0xFF
    };

    class ParseContext {

        static const size_t total_size = 34;

#if defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH do {} while (0)
#endif
#define BRK(X) if (X - _offset >= size) break; FALLTHROUGH

        public:
            size_t consume(uint8_t *data, size_t size) {
                // Mildly filty, but makes parsing in small chunks simpler
                switch (_offset) {
                    // Single byte data
                    case 0: op = data[0 - _offset]; BRK(0);
                    case 1: htype = data[1 - _offset]; BRK(1);
                    case 2: hlen = data[2 - _offset]; BRK(2);
                    case 3: hops = data[3 - _offset]; BRK(3);
                    // Xid: u32
                    case 4: xid |= (data[4 - _offset] << 24); BRK(4);
                    case 5: xid |= (data[5 - _offset] << 16); BRK(5);
                    case 6: xid |= (data[6 - _offset] <<  8); BRK(6);
                    case 7: xid |= (data[7 - _offset]); BRK(7);
                    // Secs: u16
                    case 8: secs |= (data[8 - _offset] << 8); BRK(8);
                    case 9: secs |= (data[9 - _offset]); BRK(9);
                    // Flags: u16
                    case 10: flags |= (data[10 - _offset] << 8); BRK(10);
                    case 11: flags |= (data[11 - _offset]); BRK(11);
                    // Ciaddr
                    case 12: ciaddr[0] = data[12 - _offset]; BRK(12);
                    case 13: ciaddr[1] = data[13 - _offset]; BRK(13);
                    case 14: ciaddr[2] = data[14 - _offset]; BRK(14);
                    case 15: ciaddr[3] = data[15 - _offset]; BRK(15);
                    // Yiaddr
                    case 16: yiaddr[0] = data[16 - _offset]; BRK(16);
                    case 17: yiaddr[1] = data[17 - _offset]; BRK(17);
                    case 18: yiaddr[2] = data[18 - _offset]; BRK(18);
                    case 19: yiaddr[3] = data[19 - _offset]; BRK(19);
                    // Siaddr
                    case 20: siaddr[0] = data[20 - _offset]; BRK(20);
                    case 21: siaddr[1] = data[21 - _offset]; BRK(21);
                    case 22: siaddr[2] = data[22 - _offset]; BRK(22);
                    case 23: siaddr[3] = data[23 - _offset]; BRK(23);
                    // Giaddr
                    case 24: giaddr[0] = data[24 - _offset]; BRK(24);
                    case 25: giaddr[1] = data[25 - _offset]; BRK(25);
                    case 26: giaddr[2] = data[26 - _offset]; BRK(26);
                    case 27: giaddr[3] = data[27 - _offset]; BRK(27);
                    // Chaddr
                    case 28: chaddr[0] = data[28 - _offset]; BRK(28);
                    case 29: chaddr[1] = data[29 - _offset]; BRK(29);
                    case 30: chaddr[2] = data[30 - _offset]; BRK(30);
                    case 31: chaddr[3] = data[31 - _offset]; BRK(31);
                    case 32: chaddr[4] = data[32 - _offset]; BRK(32);
                    case 33: chaddr[5] = data[33 - _offset];
                }

                // Update our offset with the number of bytes consumed
                const size_t old_offset = _offset;
                _offset += size;
                if (_offset >= total_size) {
                    _offset = total_size;
                }

                // Return the number of consume bytes
                return _offset - old_offset;
            }

        protected:
            uint8_t op = 0;
            uint8_t htype = 0;
            uint8_t hlen = 0;
            uint8_t hops = 0;
            uint32_t xid = 0;
            uint16_t secs = 0;
            uint16_t flags = 0;
            uint8_t ciaddr[4] = {0};
            uint8_t yiaddr[4] = {0};
            uint8_t siaddr[4] = {0};
            uint8_t giaddr[4] = {0};
            uint8_t chaddr[6] = {0};

            friend class Client;

        private:
            size_t _offset = 0;
    };

    class Client {
        public:
            Client(W5500& driver, const char *hostname)
                : _driver(driver), _hostname(hostname)
            {};

            void update();

        private:
            W5500& _driver;
            const char *_hostname;

            // Socket on the W5500 to be used for DHCP
            int _socket = -1;

            // Current unique transaction ID
            uint32_t _initial_xid = 0;
            uint32_t _xid = 0;

            // IP addresses
            uint8_t _local_ip[4]; // Our client IP
            uint8_t _dhcp_server_ip[4]; // IP of chosen DHCP server

            // DHCP-obtained network info
            uint8_t _subnet_mask[4];
            uint8_t _gateway_ip[4];
            uint8_t _dns_server_ip[4];

            // State machine
            State _state = State::START;

            // Timings
            uint64_t _lease_request_start = 0L;
            uint64_t _last_discover_broadcast = 0L;

            // Encoding numbers to/from buffers
            void embed_u16(uint8_t *buffer, uint16_t value);
            void embed_u32(uint8_t *buffer, uint32_t value);

            // Packet generation
            void send_dhcp_packet(DhcpMessageType type);

            // Seconds since last DHCP start
            uint16_t seconds_elapsed();

            void reset_current_lease();
            void request_lease();

            void parse_dhcp_response();

            // Finite state machine submethods
            void fsm_start();
            void fsm_discover();
    };

}}} // Namespace

#endif // #ifndef _W5500__W5500_PROTOCOLS_DHCP_H_
