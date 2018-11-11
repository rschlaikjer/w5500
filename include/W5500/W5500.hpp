#ifndef _W5500__W5500_W5500_H_
#define _W5500__W5500_W5500_H_

#include <unistd.h>
#include <string.h>

#include <initializer_list>

#include <W5500/Bus.hpp>
#include <W5500/Registers.hpp>

namespace W5500 {

    static const size_t max_sockets = 8;

    class SocketInfo {
        protected:
            uint16_t write_ptr = 0;
            uint16_t read_ptr = 0;
            Registers::Socket::BufferSize tx_buffer_size = Registers::Socket::BufferSize::SZ_2K;

            void increment_write_pointer(uint16_t amount) {
                write_ptr += amount;
            }

            void increment_read_pointer(uint16_t amount) {
                read_ptr += amount;
            }

            friend class W5500;
    };

    class W5500 {
        public:
            W5500(Bus& bus) : _bus(bus) {}
            void init();

            void reset();
            uint8_t get_version();

            void set_force_arp(bool enable);

            void set_mac(uint8_t mac[6]);
            void get_mac(uint8_t mac[6]);
            void set_gateway(uint8_t gwip[4]);
            void get_gateway(uint8_t gwip[4]);
            void set_subnet_mask(uint8_t mask[4]);
            void get_subnet_mask(uint8_t mask[4]);
            void set_ip(uint8_t ip[4]);
            void get_ip(uint8_t ip[4]);

            bool link_up();
            Registers::Socket::StatusValue get_socket_status(uint8_t socket);
            uint8_t get_socket_interrupts(uint8_t socket);

            void set_interrupt_mask(
                std::initializer_list<Registers::Common::InterruptMaskFlags> flags);
            uint8_t get_interrupt_state();
            bool has_interrupt_flag(Registers::Common::InterruptMaskFlags flag);

            void set_socket_mode(uint8_t socket, SocketMode mode);
            void send_socket_command(uint8_t socket, Registers::Socket::CommandValue command);
            void set_socket_dest_ip_address(uint8_t socket, uint8_t target_ip[4]);
            void set_socket_dest_mac(uint8_t socket, uint8_t mac[6]);
            void get_socket_dest_mac(uint8_t socket, uint8_t mac[6]);
            void set_socket_dest_port(uint8_t socket, uint16_t port);
            void set_socket_src_port(uint8_t socket, uint16_t port);

            // Socket TX/RX handling
            uint16_t get_tx_free_size(uint8_t socket);
            uint16_t get_tx_read_pointer(uint8_t socket);
            uint16_t get_tx_write_pointer(uint8_t socket);
            uint16_t get_rx_byte_count(uint8_t socket);
            uint16_t get_rx_read_pointer(uint8_t socket);
            uint16_t get_rx_write_pointer(uint8_t socket);

            // Socket interrupts
            Registers::Socket::InterruptRegisterValue get_socket_interrupt_flags(uint8_t socket);
            bool socket_has_interrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag);
            void clear_socket_iterrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag);

            // Trigger a flush of data written to buffer
            void send(uint8_t socket);
            // Write data to buffer and immediately trigger send
            size_t send(uint8_t socket, const uint8_t *buffer, size_t size);
            // Write data to buffer but do NOT automatically trigger send
            size_t write(uint8_t socket, const uint8_t *buffer, size_t size);

        private:
            Bus& _bus;

            // Local socket info
            SocketInfo _socket_info[max_sockets];

            void write_register(CommonRegister reg, uint8_t *data);
            void write_register(SocketRegister reg, uint8_t socket_n, uint8_t *data);
            void write_register_u16(SocketRegister reg, uint8_t socket, uint16_t value);
            void read_register(CommonRegister reg, uint8_t *data);
            void read_register(SocketRegister reg, uint8_t socket_n, uint8_t *data);
            uint16_t read_register_u16(SocketRegister reg, uint8_t socket);
    };

}

#endif // #ifndef _W5500__W5500_W5500_H_
