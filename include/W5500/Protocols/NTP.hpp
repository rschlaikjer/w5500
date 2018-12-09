#ifndef _W5500__W5500_PROTOCOLS_NTP_H_
#define _W5500__W5500_PROTOCOLS_NTP_H_

#include <stdint.h>

#include <W5500/W5500.hpp>

namespace W5500 {
    namespace Protocols {
        namespace NTP {

    static const uint16_t ntp_port = 123;
    static const uint16_t ntp_packet_size = 48;
    static const uint32_t seventy_years = 2208988800UL;
    static const uint64_t request_interval_ms = 30000;

    enum class LI : uint8_t {
        NO_WARNING = 0x00,
        LEAP_61_SECONDS = 0x01,
        LEAP_59_SECONDS = 0x02,
        ALARM = 0x03
    };

    enum class Mode : uint8_t {
        RESERVED = 0x00,
        SYMMETRIC_ACTIVE = 0x01,
        SYMMETRIC_PASSIVE = 0x02,
        CLIENT = 0x03,
        SERVER = 0x04,
        BROADCAST = 0x05,
        RESERVED_CONTROL = 0x06,
        RESERVED_PRIVATE = 0x07
    };

    enum class Stratum : uint8_t {
        KISS_O_DEATH = 0x00,
        PRIMARY_REFERNCE = 0x01,
        SECONDARY_REFERENCE = 0x02,
        SECONDARY_REFERENCE_END = 0x0F,
        RESERVED = 0x10
    };

#define REFSOURCE3(A, B, C) ((A << 16) | (B << 8) | C)
#define REFSOURCE4(A, B, C, D) ((A << 24) | (B << 16) | (C << 8) | D)

    enum class ReferenceSource : uint32_t {
      LOCAL_CLOCK = REFSOURCE4('L', 'O', 'C', 'L'),
      CESIUM_CLOCK = REFSOURCE4('C', 'E', 'S', 'M'),
      RUBIDIUM_CLOCK = REFSOURCE4('R', 'B', 'D', 'M'),
      PULSE_PER_SECOND = REFSOURCE3('P', 'P', 'S'),
      INTER_RANGE_INSTRUMENTATION_GROUP = REFSOURCE4('I', 'R', 'I', 'G'),
      NIST_MODEM_SERVICE = REFSOURCE4('A', 'C', 'T', 'S'),
      USNO_MODEM_SERVICE = REFSOURCE4('U', 'S', 'N', 'O'),
      PTB_MODEM_SERVICE = REFSOURCE3('P', 'T', 'B'),
      ALLOUIS_RADIO = REFSOURCE3('T', 'D', 'F'),
      MAINFLINGEN_RADIO = REFSOURCE3('D', 'C', 'F'),
      RUGBY_RADIO = REFSOURCE3('M', 'S', 'F'),
      FT_COLLINS_RADIO = REFSOURCE3('W', 'W', 'V'),
      BOULDER_RADIO = REFSOURCE4('W', 'W', 'V', 'B'),
      KAUAI_RADIO = REFSOURCE4('W', 'W', 'V', 'H'),
      OTTOWA_RADIO = REFSOURCE3('C', 'H', 'U'),
      LORAN_C = REFSOURCE4('L', 'O', 'R', 'C'),
      OMEGA = REFSOURCE4('O', 'M', 'E', 'G'),
      GPS = REFSOURCE3('G', 'P', 'S')
    };

    class Client {
        public:
            Client(W5500& driver, UdpSocket& socket,
                   uint8_t a=0, uint8_t b=0, uint8_t c=0, uint8_t d=0)
                : _driver(driver), _socket(socket) {
                _ip[0] = a;
                _ip[1] = b;
                _ip[2] = c;
                _ip[3] = d;
            }

            bool update(uint64_t *current_time);
            void set_server_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

        private:
            W5500& _driver;
            UdpSocket& _socket;

            // NTP server IP
            uint8_t _ip[4];

            // Time since last NTP request
            uint64_t _last_ntp_request = 0L;

            // Time since last successful NTP lock
            uint64_t _last_ntp_response = 0L;

            // Poll interval once locked, in log2 seconds
            uint8_t _poll_interval = 1;

            bool parse_packet(uint64_t *current_time);
            void send_request();

    };

}}} // Namespace

#endif // #ifndef _W5500__W5500_PROTOCOLS_NTP_H_
