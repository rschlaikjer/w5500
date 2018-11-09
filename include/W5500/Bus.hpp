#ifndef _W5500__W5500_BUS_H_
#define _W5500__W5500_BUS_H_

#include <stdint.h>

namespace W5500 {

    class W5500;

    class Bus {
        public:
            // SPI read/write method. Must be implemented by end user
            virtual void spi_xfer(uint8_t send, uint8_t *recv) = 0;

            // Utility method for one-way transfers
            void spi_xfer(uint8_t send) {
                spi_xfer(&send, nullptr, 1);
            }

            // Send/receive count bytes.
            // If send is nullptr, zeros will be sent.
            // If recv is nullptr, received data will be ignored.
            // If both arrays are present, both _must_ be of at least size count
            void spi_xfer(uint8_t *send, uint8_t *recv, size_t count) {
                // If you're not sending or receiving anything, why are you here
                if (send == nullptr && recv == nullptr) {
                    return;
                }

                if (send != nullptr && recv != nullptr) {
                    // Send & recv non-null
                    for (size_t i = 0; i < count; i++) {
                        spi_xfer(send[i], &recv[i]);
                    }
                } else if (send != nullptr) {
                    // Recv is null, send only
                    uint8_t dummy;
                    for (size_t i = 0; i < count; i++) {
                        spi_xfer(send[i], &dummy);
                    }
                } else {
                    // Send is null, recv only
                    for (size_t i = 0; i < count; i++) {
                        spi_xfer(0x0, &recv[i]);
                    }
                }
            }

            // Chip select pin manipulation
            virtual void chip_select() = 0;
            virtual void chip_deselect() = 0;

            // Interrupt handling.
            // Attach an interrupt using your target framework, and call
            // this method from it.
            void trigger_interrupt() {
                _interrupt_pending = true;
            }
            bool has_pending_interrupt() {
                return _interrupt_pending;
            }

        protected:
            void clear_interrupt_flag() {
                _interrupt_pending = false;
            }
            friend class W5500;

        private:
            bool _interrupt_pending = false;
    };

}

#endif // #ifndef _W5500__W5500_BUS_H_
