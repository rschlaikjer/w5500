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

    class Client {
        public:
            Client(W5500& driver, const char *hostname)
                : _driver(driver), _hostname(hostname)
            {};

            void discover();

        private:
            W5500& _driver;
            const char *_hostname;

            // Socket on the W5500 to be used for DHCP
            uint8_t _socket = 0;

            // Current unique transaction ID
            uint32_t _xid = 0;

            // IP addresses
            uint8_t _local_ip[4]; // Our client IP
            uint8_t _dhcp_server_ip[4]; // IP of chosen DHCP server

            // Encoding numbers to/from buffers
            void embed_u16(uint8_t *buffer, uint16_t value);
            void embed_u32(uint8_t *buffer, uint32_t value);

            // Packet generation
            void send_dhcp_packet(DhcpMessageType type);

            // Seconds since last DHCP start
            uint16_t seconds_elapsed();
    };

}}} // Namespace

#endif // #ifndef _W5500__W5500_PROTOCOLS_DHCP_H_
