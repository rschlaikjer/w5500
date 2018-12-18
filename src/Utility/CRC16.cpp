#include <W5500/Utility/CRC16.hpp>

namespace W5500 {
    namespace Utility {
        namespace CRC16 {

uint16_t update(uint16_t crc, uint8_t data) {
    return (crc << 8) ^ table[((crc >> 8) ^ data) & 0x00FF];
}

uint16_t of(const uint8_t *data, size_t len) {
    uint16_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = update(crc, data[i]);
    }
    return crc;
}

}}} // Namespace
