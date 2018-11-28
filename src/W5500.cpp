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

bool W5500::link_up() {
    uint8_t val;
    read_register(Registers::Common::PhyConfig, &val);
    return val & static_cast<uint8_t>(Registers::Common::PhyConfigFlags::LINK_STATUS);
}

Registers::Socket::StatusValue W5500::get_socket_status(uint8_t socket) {
    return Registers::Socket::StatusValue(
        read_register_u8(Registers::Socket::Status, socket));
}

void W5500::set_socket_mode(uint8_t socket, SocketMode mode) {
    write_register_u8(Registers::Socket::Mode, socket, static_cast<uint8_t>(mode));
}

void W5500::send_socket_command(uint8_t socket, Registers::Socket::CommandValue command) {
    write_register_u8(Registers::Socket::Command, socket, static_cast<uint8_t>(command));
}

void W5500::set_socket_dest_ip_address(uint8_t socket, const uint8_t target_ip[4]) {
    write_register(Registers::Socket::DestIPAddress, socket, target_ip);
}

void W5500::set_socket_dest_port(uint8_t socket, uint16_t port) {
    write_register_u16(Registers::Socket::DestPort, socket, port);
}

void W5500::set_socket_src_port(uint8_t socket, uint16_t port) {
    write_register_u16(Registers::Socket::SourcePort, socket, port);
}

void W5500::reset() {
    // Set soft reset bit
    uint8_t flag = static_cast<uint8_t>(Registers::Common::ModeFlags::RESET);
    write_register(Registers::Common::Mode, &flag);

    // Wait for core reset to complete
    do {
        read_register(Registers::Common::Mode, &flag);
    } while (flag & static_cast<uint8_t>(Registers::Common::ModeFlags::RESET));

    // Wait for PHY reset to complete
    do {
        read_register(Registers::Common::PhyConfig, &flag);
    } while (!(flag & static_cast<uint8_t>(Registers::Common::PhyConfigFlags::RESET)));
}

void W5500::set_force_arp(bool enable) {
    // Get current reg value
    uint8_t flag = read_register_u8(Registers::Common::Mode);

    // Set/clear FARP bit
    if (enable) {
        flag |= static_cast<uint8_t>(Registers::Common::ModeFlags::FORCE_ARP);
    } else {
        flag &= ~static_cast<uint8_t>(Registers::Common::ModeFlags::FORCE_ARP);
    }

    // Write register back
    write_register_u8(Registers::Common::Mode, flag);
}

void W5500::set_socket_dest_mac(uint8_t socket, const uint8_t mac[6]) {
    write_register(Registers::Socket::DestHardwareAddress, socket, mac);
}

void W5500::get_socket_dest_mac(uint8_t socket, uint8_t mac[6]) {
    read_register(Registers::Socket::DestHardwareAddress, socket, mac);
}

void W5500::set_socket_buffer_size(uint8_t socket, Registers::Socket::BufferSize size) {
    set_socket_tx_buffer_size(socket, size);
    set_socket_rx_buffer_size(socket, size);
}

Registers::Socket::BufferSize W5500::get_socket_tx_buffer_size(uint8_t socket) {
    return Registers::Socket::BufferSize(read_register_u8(Registers::Socket::TxBufferSize, socket));
}

Registers::Socket::BufferSize W5500::get_socket_rx_buffer_size(uint8_t socket) {
    return Registers::Socket::BufferSize(read_register_u8(Registers::Socket::RxBufferSize, socket));
}

void W5500::set_socket_tx_buffer_size(uint8_t socket, Registers::Socket::BufferSize size) {
    write_register_u8(Registers::Socket::TxBufferSize, socket, static_cast<uint8_t>(size));
}

void W5500::set_socket_rx_buffer_size(uint8_t socket, Registers::Socket::BufferSize size) {
    write_register_u8(Registers::Socket::RxBufferSize, socket, static_cast<uint8_t>(size));
}

void W5500::write_register(CommonRegister reg, const uint8_t *data) {
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

void W5500::write_register(SocketRegister reg, uint8_t socket_n, const uint8_t *data) {
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
    write_register_u8(Registers::Common::InterruptMask, mask);
}

Registers::Common::InterruptRegisterValue W5500::get_interrupt_state() {
    return Registers::Common::InterruptRegisterValue(
        read_register_u8(Registers::Common::Interrupt));
}

bool W5500::has_interrupt_flag(Registers::Common::InterruptFlags flag) {
    return get_interrupt_state() & flag;
}

void W5500::clear_interrupt_flag(Registers::Common::InterruptFlags flag) {
    write_register_u8(Registers::Common::Interrupt, static_cast<uint8_t>(flag));
}

uint8_t W5500::get_version() {
    return read_register_u8(Registers::Common::ChipVersion);
}

void W5500::send(uint8_t socket) {
    // Trigger a send.
    send_socket_command(socket, Registers::Socket::CommandValue::SEND);
}

size_t W5500::send(uint8_t socket, const uint8_t *buffer, size_t offset, size_t size) {
    // Send with arguments: copy the data to the IC using write(), then
    // immediately trigger a flush.
    const size_t written = write(socket, buffer, offset, size);
    send_socket_command(socket, Registers::Socket::CommandValue::SEND);
    return written;
}

size_t W5500::write(uint8_t socket, const uint8_t *buffer, size_t offset, size_t size) {
    // Get max possible tx size
    const uint16_t free_buffer_size = get_tx_free_size(socket);

    // If the buffer is full just don't even try
    if (free_buffer_size == 0) {
        return 0;
    }

    // Set up the write transaction
    uint8_t cmd[3];
    const uint16_t write_pointer = get_tx_write_pointer(socket);
    const uint16_t write_offset = write_pointer + offset;
    cmd[0] = (write_offset >> 8) & 0xFF;
    cmd[1] = write_offset & 0xFF;
    cmd[2] = (
        (SOCKET_TX_BUFFER(socket) << 3) |
        (1 << 2) | // Write
        0x0 // Always use VDM mode
    );
    _bus.chip_select();

    // Initiate the write
    _bus.spi_xfer(cmd, nullptr, 3);

    // Send as much data as we can
    const uint16_t bytes_to_send = (size <= free_buffer_size ? size : free_buffer_size);
    _bus.spi_xfer(buffer, nullptr, bytes_to_send);

    // Done
    _bus.chip_deselect();

    // Update the socket TX write pointer register
    set_tx_write_pointer(socket, write_offset + bytes_to_send);

    // Return the amount of bytes that were actually sent
    return bytes_to_send;
}

uint16_t W5500::read_register_u8(CommonRegister reg) {
    uint8_t val;
    read_register(reg, &val);
    return val;
}

uint16_t W5500::read_register_u8(SocketRegister reg, uint8_t socket) {
    uint8_t val;
    read_register(reg, socket, &val);
    return val;
}

size_t W5500::peek(uint8_t socket, uint8_t *buffer, size_t size) {
    // Set up the read transaction
    uint8_t cmd[3];
    uint16_t read_offset = get_rx_read_pointer(socket);
    cmd[0] = (read_offset >> 8) & 0xFF;
    cmd[1] = read_offset & 0xFF;
    cmd[2] = (
        (SOCKET_RX_BUFFER(socket) << 3) |
        (0 << 2) | // Read
        0x0 // Always use VDM mode
    );
    _bus.chip_select();

    // Initiate the read
    _bus.spi_xfer(cmd, nullptr, 3);

    // Read the data
    _bus.spi_xfer(nullptr, buffer, size);

    // Done
    _bus.chip_deselect();

    // Return the amount of bytes that were actually sent
    return size;
}

uint16_t W5500::read_register_u16(CommonRegister reg) {
    uint8_t buf[2];
    read_register(reg, buf);
    return buf[0] << 8 | buf[1];
}

uint8_t W5500::read(uint8_t socket) {
    uint8_t val;
    read(socket, &val, 1);
    return val;
}

size_t W5500::read(uint8_t socket, uint8_t *buffer, size_t size) {
    // Check if the receive buffer is valid, if it's null we
    // want to just skip data
    size_t read;
    if (buffer != nullptr) {
        // Read the data from the IC
        read = peek(socket, buffer, size);
    } else {
        // Don't bother to read, just advance read pointer
        read = size;
    }

    // Update the socket RX read pointer register
    set_rx_read_pointer(socket, get_rx_read_pointer(socket) + read);

    // Call RECV to update the chip state
    send_socket_command(socket, Registers::Socket::CommandValue::RECV);

    return read;
}

size_t W5500::flush(uint8_t socket) {
    // Get the pending data size
    const uint16_t read_ptr = get_rx_read_pointer(socket);
    const uint16_t write_ptr = get_rx_write_pointer(socket);

    // Update the socket RX read pointer register
    set_rx_read_pointer(socket, write_ptr);

    // Call RECV to update the chip state
    send_socket_command(socket, Registers::Socket::CommandValue::RECV);

    // Return the number of discarded bytes
    return write_ptr - read_ptr;
}

uint16_t W5500::read_register_u16(SocketRegister reg, uint8_t socket) {
    uint8_t buf[2];
    read_register(reg, socket, buf);
    return buf[0] << 8 | buf[1];
}

void W5500::write_register_u8(CommonRegister reg, uint8_t val) {
    write_register(reg, &val);
}

void W5500::write_register_u8(SocketRegister reg, uint8_t socket, uint8_t val) {
    write_register(reg, socket, &val);
}

void W5500::write_register_u16(SocketRegister reg, uint8_t socket, uint16_t value) {
    uint8_t buf[2];
    buf[0] = (value >> 8) & 0xFF;
    buf[1] = value & 0xFF;
    write_register(reg, socket, buf);
}

uint16_t W5500::get_tx_free_size(uint8_t socket) {
    return read_register_u16(Registers::Socket::TxFreeSize, socket);
}

uint16_t W5500::get_tx_read_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::TxReadPointer, socket);
}

uint16_t W5500::get_tx_write_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::TxWritePointer, socket);
}

void W5500::set_tx_write_pointer(uint8_t socket, uint16_t offset) {
    write_register_u16(Registers::Socket::TxWritePointer, socket, offset);
}

uint16_t W5500::get_rx_byte_count(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxReceivedSize, socket);
}

uint16_t W5500::get_rx_read_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxReadPointer, socket);
}

void W5500::set_rx_read_pointer(uint8_t socket, uint16_t offset) {
    return write_register_u16(Registers::Socket::RxReadPointer, socket, offset);
}

uint16_t W5500::get_rx_write_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxWritePointer, socket);
}

Registers::Socket::InterruptRegisterValue W5500::get_socket_interrupt_flags(uint8_t socket) {
    const uint8_t val = read_register_u8(Registers::Socket::Interrupt, socket);
    return Registers::Socket::InterruptRegisterValue(val);
}

bool W5500::socket_has_interrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag) {
    return get_socket_interrupt_flags(socket) & flag;
}

void W5500::clear_socket_interrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag) {
    write_register_u8(Registers::Socket::Interrupt, socket, static_cast<uint8_t>(flag));
}

} // namespace W5500
