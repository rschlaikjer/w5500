#ifndef _W5500__W5500_SOCKET_H_
#define _W5500__W5500_SOCKET_H_

#include <stdint.h>
#include <unistd.h>

#include <W5500/Registers.hpp>

namespace W5500 {

    // Forward-declare driver class
    class W5500;

    class Socket {
        public:
            Socket(W5500& driver, uint8_t sockfd) :
                _driver(driver), _sockfd(sockfd) {}
            virtual ~Socket() {}

            virtual bool init() = 0;
            virtual bool ready() = 0;
            bool phy_link_up();

            void set_dest_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
            void set_dest_ip(const uint8_t ip[4]);
            void set_dest_port(uint16_t port);
            void set_source_port(uint16_t port);

            void close();

            Registers::Socket::InterruptRegisterValue get_interrupt_flags();
            void clear_interrupt_flag(Registers::Socket::InterruptFlags val);

            // Must be called after a TCP conn is opened
            void update_buffer_offsets();

            virtual uint8_t read();
            virtual int read(uint8_t *buffer, size_t size);
            virtual void flush();

            int write(const uint8_t *buffer, size_t size);
            int send(const uint8_t *buffer, size_t size);
            void send();

        protected:
            W5500& _driver;
            const uint8_t _sockfd;

            void connect();

        private:
            // Disallow copying of sockets
            Socket(const Socket&);
            Socket& operator=(const Socket&);
    };

    class UdpSocket : public Socket {
        public:
            using Socket::Socket;

            bool init() override;
            bool ready() override;

            bool has_packet();
            int peek_packet(uint8_t source_ip[4], uint16_t& source_port);
            int read_packet_header(uint8_t source_ip[4], uint16_t& source_port);

            uint8_t read() override;
            int read(uint8_t *buffer, size_t size) override;
            void flush() override;

            int remaining_bytes_in_packet();
            void skip_to_packet_end();

        private:
            int _packet_bytes_remaining = 0;
    };

    class TcpSocket : public Socket {
        public:
            using Socket::Socket;

            uint16_t _ephemeral_port = 1;

            bool init() override;
            bool ready() override;
            bool connecting();
            bool connected();

            void connect(const uint8_t ip[4], uint16_t port);
    };

}

#endif // #ifndef _W5500__W5500_SOCKET_H_
