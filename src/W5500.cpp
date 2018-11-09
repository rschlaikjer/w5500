#include <W5500/W5500.hpp>

namespace W5500 {

void W5500::init() {
    _bus.init();
}

void W5500::set_mac(uint8_t mac[6]) {
    write_register(Registers::Common::SourceHardwareAddress, mac);
}

void W5500::set_gateway(uint8_t ip[4]) {
    write_register(Registers::Common::GatewayAddress, ip);
}

void W5500::set_subnet_mask(uint8_t mask[4]) {
    write_register(Registers::Common::SubnetMaskAddress, mask);
}

void W5500::set_ip(uint8_t ip[4]) {
    write_register(Registers::Common::SourceIpAddress, ip);
}

void W5500::get_mac(uint8_t mac[6]) {
    read_register(Registers::Common::SourceHardwareAddress, mac);
}

void W5500::get_gateway(uint8_t ip[4]) {
    read_register(Registers::Common::GatewayAddress, ip);
}

void W5500::get_subnet_mask(uint8_t mask[4]) {
    read_register(Registers::Common::SubnetMaskAddress, mask);
}

void W5500::get_ip(uint8_t ip[4]) {
    read_register(Registers::Common::SourceIpAddress, ip);
}

void W5500::write_register(CommonRegister reg, uint8_t *data) {
    const uint8_t cmd_size = 3 + reg.size;
    uint8_t cmd[cmd_size];
    // Set the register offset
    cmd[0] = 0x0;
    cmd[1] = reg.offset;
    // Control byte = bank select + R/W + OP mode
    cmd[2] = (
        (COMMON_REGISTER_BANK << 3) |
        (1 << 2) | // Write
        0x0 // Always use VDM mode
    );
    // Copy data to write to cmd buffer
    memcpy(&cmd[3], data, reg.size);
    // Write data to bus
    _bus.chip_select();
    _bus.spi_xfer(cmd, nullptr, cmd_size);
    _bus.chip_deselect();
}

void W5500::write_register(SocketRegister reg, uint8_t socket_n, uint8_t *data) {
    const uint8_t cmd_size = 3 + reg.size;
    uint8_t cmd[cmd_size];
    // Set the register offset
    cmd[0] = 0x0;
    cmd[1] = reg.offset;
    // Control byte = bank select + R/W + OP mode
    cmd[2] = (
        (SOCKET_REG(socket_n) << 3) |
        (1 << 2) | // Write
        0x0 // Always use VDM mode
    );
    // Copy data to write to cmd buffer
    memcpy(&cmd[3], data, reg.size);
    // Write data to bus
    _bus.chip_select();
    _bus.spi_xfer(cmd, nullptr, cmd_size);
    _bus.chip_deselect();
}

void W5500::read_register(CommonRegister reg, uint8_t *data) {
    // Send the read command
    uint8_t cmd[3];
    cmd[0] = 0x0;
    cmd[1] = reg.offset;
    cmd[2] = (
        (COMMON_REGISTER_BANK << 3) |
        (0 << 2) | // Read
        0x0 // Always use VDM mode
    );
    _bus.chip_select();
    _bus.spi_xfer(cmd, nullptr, 3);

    // Read out the response
    _bus.spi_xfer(nullptr, data, reg.size);
    _bus.chip_deselect();
}

void W5500::read_register(SocketRegister reg, uint8_t socket_n, uint8_t *data) {
    // Send the read command
    uint8_t cmd[3];
    cmd[0] = 0x0;
    cmd[1] = reg.offset;
    cmd[2] = (
        (SOCKET_REG(socket_n) << 3) |
        (0 << 2) | // Read
        0x0 // Always use VDM mode
    );
    _bus.chip_select();
    _bus.spi_xfer(cmd, nullptr, 3);

    // Read out the response
    _bus.spi_xfer(nullptr, data, reg.size);
    _bus.chip_deselect();
}

void W5500::set_interrupt_mask(
        std::initializer_list<Registers::Common::InterruptMaskFlags> flags) {
    uint8_t mask = 0x0;
    for (auto flag : flags) {
        mask |= static_cast<uint8_t>(flag);
    }
    write_register(Registers::Common::InterruptMask, &mask);
}

bool W5500::has_interrupt_flag(Registers::Common::InterruptMaskFlags flag) {

}

uint8_t W5500::get_version() {
    uint8_t version;
    read_register(Registers::Common::ChipVersion, &version);
    return version;
}
}
