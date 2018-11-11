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

uint8_t W5500::get_interrupt_state() {
    uint8_t val;
    read_register(Registers::Common::Interrupt, &val);
    return val;
}

bool W5500::link_up() {
    uint8_t val;
    read_register(Registers::Common::PhyConfig, &val);
    return val & static_cast<uint8_t>(Registers::Common::PhyConfigFlags::LINK_STATUS);
}

Registers::Socket::StatusValue W5500::get_socket_status(uint8_t socket) {
    uint8_t val;
    read_register(Registers::Socket::Status, socket, &val);
    return Registers::Socket::StatusValue(val);
}

int W5500::open_socket(SocketMode mode) {
    size_t sock;
    // Find first open socket
    for (sock = 0; sock < max_sockets; sock++) {
        if (!_socket_info[sock].opened) {
            break;
        }
    }

    // All sockets open, cannot open another
    if (sock == max_sockets) {
        return -1;
    }

    // Mark socket as open
    _socket_info[sock].opened = true;

    // Set the socket mode
    set_socket_mode(sock, mode);

    // Ask the W5500 to actually open the socket
    send_socket_command(sock, Registers::Socket::CommandValue::OPEN);

    // Return the socket id
    return sock;
}

void W5500::close_socket(uint8_t sock) {
    // Bounds check
    if (sock >= max_sockets) {
        return;
    }

    // If socket not open, do nothing
    if (!_socket_info[sock].opened) {
        return;
    }

    // Ask W5500 to close socket
    send_socket_command(sock, Registers::Socket::CommandValue::CLOSE);

    // Mark socket closed
    _socket_info[sock].opened = false;
}

void W5500::set_socket_mode(uint8_t socket, SocketMode mode) {
    uint8_t val = static_cast<uint8_t>(mode);
    write_register(Registers::Socket::Mode, socket, &val);
}

void W5500::send_socket_command(uint8_t socket, Registers::Socket::CommandValue command) {
    uint8_t val = static_cast<uint8_t>(command);
    write_register(Registers::Socket::Command, socket, &val);
}

void W5500::set_socket_dest_ip_address(uint8_t socket, uint8_t target_ip[4]) {
    write_register(Registers::Socket::DestIPAddress, socket, target_ip);
}

void W5500::set_socket_dest_port(uint8_t socket, uint16_t port) {
    uint8_t port_8[2];
    port_8[0] = (port >> 8) & 0xFF;
    port_8[1] = port & 0xFF;
    write_register(Registers::Socket::DestPort, socket, port_8);
}

void W5500::set_socket_src_port(uint8_t socket, uint16_t port) {
    uint8_t port_8[2];
    port_8[0] = (port >> 8) & 0xFF;
    port_8[1] = port & 0xFF;
    write_register(Registers::Socket::SourcePort, socket, port_8);
}

void W5500::reset() {
    // Set soft reset bit
    uint8_t flag = static_cast<uint8_t>(Registers::Common::ModeFlags::RESET);
    write_register(Registers::Common::Mode, &flag);

    // Wait for reset to complete
    do {
        read_register(Registers::Common::Mode, &flag);
    } while (flag & static_cast<uint8_t>(Registers::Common::ModeFlags::RESET));
}

void W5500::set_force_arp(bool enable) {
    // Get current reg value
    uint8_t flag;
    read_register(Registers::Common::Mode, &flag);

    // Set/clear FARP bit
    if (enable) {
        flag |= static_cast<uint8_t>(Registers::Common::ModeFlags::FORCE_ARP);
    } else {
        flag &= ~static_cast<uint8_t>(Registers::Common::ModeFlags::FORCE_ARP);
    }

    // Write register back
    write_register(Registers::Common::Mode, &flag);
}

void W5500::set_socket_dest_mac(uint8_t socket, uint8_t mac[6]) {
    write_register(Registers::Socket::DestHardwareAddress, socket, mac);
}

void W5500::get_socket_dest_mac(uint8_t socket, uint8_t mac[6]) {
    read_register(Registers::Socket::DestHardwareAddress, socket, mac);
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
    return false;
}

uint8_t W5500::get_version() {
    uint8_t version;
    read_register(Registers::Common::ChipVersion, &version);
    return version;
}

void W5500::send(uint8_t socket) {
    // Send with no args: flush any pending data in the TX buffer.
    // If the tx read pointer == the write pointer, a send would send nothing.
    if (get_tx_read_pointer(socket) == _socket_info[socket].write_ptr) {
        return;
    }

    // Trigger a send.
    send_socket_command(socket, Registers::Socket::CommandValue::SEND);
}

size_t W5500::send(uint8_t socket, const uint8_t *buffer, size_t size) {
    // Send with arguments: copy the data to the IC using write(), then
    // immediately trigger a flush.
    const size_t written = write(socket, buffer, size);
    send_socket_command(socket, Registers::Socket::CommandValue::SEND);
    return written;
}

size_t W5500::write(uint8_t socket, const uint8_t *buffer, size_t size) {
    // Get max possible tx size
    const uint16_t free_buffer_size = get_tx_free_size(socket);

    // If the buffer is full just don't even try
    if (free_buffer_size == 0) {
        return 0;
    }

    // Set up the write transaction
    uint8_t cmd[3];
    uint16_t write_offset = _socket_info[socket].write_ptr;
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

    // Increment local write pointer
    _socket_info[socket].increment_write_pointer(bytes_to_send);

    // Done
    _bus.chip_deselect();

    // Update the socket TX write pointer register
    write_register_u16(Registers::Socket::TxWritePointer, socket, _socket_info[socket].write_ptr);

    // Return the amount of bytes that were actually sent
    return bytes_to_send;
}

uint16_t W5500::read_register_u16(SocketRegister reg, uint8_t socket) {
    uint8_t buf[2];
    read_register(reg, socket, buf);
    return buf[0] << 8 | buf[1];
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

uint16_t W5500::get_rx_byte_count(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxReceivedSize, socket);
}

uint16_t W5500::get_rx_read_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxReadPointer, socket);
}

uint16_t W5500::get_rx_write_pointer(uint8_t socket) {
    return read_register_u16(Registers::Socket::RxWritePointer, socket);
}

Registers::Socket::InterruptRegisterValue W5500::get_socket_interrupt_flags(uint8_t socket) {
    uint8_t val;
    read_register(Registers::Socket::Interrupt, socket, &val);
    return Registers::Socket::InterruptRegisterValue(val);
}

bool W5500::socket_has_interrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag) {
    return get_socket_interrupt_flags(socket) & flag;
}

void W5500::clear_socket_iterrupt_flag(uint8_t socket, Registers::Socket::InterruptFlags flag) {
    uint8_t val = static_cast<uint8_t>(flag);
    write_register(Registers::Socket::Interrupt, socket, &val);
}

} // namespace W5500
