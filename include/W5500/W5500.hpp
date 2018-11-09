#ifndef _W5500__W5500_W5500_H_
#define _W5500__W5500_W5500_H_

#include <unistd.h>
#include <string.h>

#include <initializer_list>

#include <W5500/Bus.hpp>
#include <W5500/Registers.hpp>

namespace W5500 {

    class W5500 {
        public:
            W5500(Bus& bus) : _bus(bus) {}
            void init();

            uint8_t get_version();

            void set_mac(uint8_t mac[6]);
            void get_mac(uint8_t mac[6]);
            void set_gateway(uint8_t gwip[4]);
            void get_gateway(uint8_t gwip[4]);
            void set_subnet_mask(uint8_t mask[4]);
            void get_subnet_mask(uint8_t mask[4]);
            void set_ip(uint8_t ip[4]);
            void get_ip(uint8_t ip[4]);

            void set_interrupt_mask(
                std::initializer_list<Registers::Common::InterruptMaskFlags> flags);
            bool has_interrupt_flag(Registers::Common::InterruptMaskFlags flag);

        private:
            Bus& _bus;

            void write_register(CommonRegister reg, uint8_t *data);
            void write_register(SocketRegister reg, uint8_t socket_n, uint8_t *data);
            void read_register(CommonRegister reg, uint8_t *data);
            void read_register(SocketRegister reg, uint8_t socket_n, uint8_t *data);
    };

}

#endif // #ifndef _W5500__W5500_W5500_H_
