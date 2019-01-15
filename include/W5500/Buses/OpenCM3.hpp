#ifndef _W5500__W5500_BUSES_OPENCM3_H_
#define _W5500__W5500_BUSES_OPENCM3_H_

#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include <W5500/Bus.hpp>

namespace W5500 {
    namespace Buses {

    class OpenCM3 : public Bus {
        public:
            OpenCM3(uint32_t spi, uint32_t cs_port, uint32_t cs_pin) :
                _spi(spi), _cs_port(cs_port), _cs_pin(cs_pin) {
            }
            ~OpenCM3() override {};

            void init() override {
                Bus::init();
                gpio_mode_setup(_cs_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, _cs_pin);
                chip_deselect();
            };

            // SPI read/write method
            void spi_xfer(uint8_t send, uint8_t *recv) override {
                // Wait for TX available
                while (!(SPI_SR(_spi) & SPI_SR_TXE));

                // Set the data to send
                SPI_DR8(_spi) = send;

                // Wait for RX complete
                while (!(SPI_SR(_spi) & SPI_SR_RXNE));

                // Read the response data
                *recv = SPI_DR8(_spi);
            }

            // Chip select pin manipulation
            void chip_select() override {
                gpio_clear(_cs_port, _cs_pin);
            }
            virtual void chip_deselect() override {
                gpio_set(_cs_port, _cs_pin);
            }
        private:
            const uint32_t _spi;
            const uint32_t _cs_port;
            const uint32_t _cs_pin;
    };
    }
}

#endif // #ifndef _W5500__W5500_BUSES_OPENCM3_H_
