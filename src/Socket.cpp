#include <W5500/Socket.hpp>
#include <W5500/W5500.hpp>

namespace W5500 {

const uint16_t udp_header_size = 8;

void Socket::set_dest_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    uint8_t ip[4];
    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;
    set_dest_ip(ip);
}

void Socket::set_dest_ip(const uint8_t ip[4]) {
    _driver.set_socket_dest_ip_address(_sockfd, ip);
}

void Socket::set_dest_port(uint16_t port) {
    _driver.set_socket_dest_port(_sockfd, port);
}

void Socket::set_source_port(uint16_t port) {
    _driver.set_socket_src_port(_sockfd, port);
}

void Socket::connect() {
    _driver.send_socket_command(_sockfd,
                                Registers::Socket::CommandValue::CONNECT);
}

void Socket::close() {
    _driver.send_socket_command(_sockfd,
                                Registers::Socket::CommandValue::CLOSE);
}

Registers::Socket::InterruptRegisterValue Socket::get_interrupt_flags() {
    return _driver.get_socket_interrupt_flags(_sockfd);
}

void Socket::clear_interrupt_flag(Registers::Socket::InterruptFlags val) {
    return _driver.clear_socket_interrupt_flag(_sockfd, val);
}

void Socket::flush() { _driver.flush(_sockfd); }

uint8_t Socket::read() { return _driver.read(_sockfd); }

int Socket::peek(uint8_t *buffer, size_t size) {
    return _driver.peek(_sockfd, buffer, size);
}

int Socket::read(uint8_t *buffer, size_t size) {
    return _driver.read(_sockfd, buffer, size);
}

int Socket::write(const uint8_t *buffer, size_t size) {
    const int ret = _driver.write(_sockfd, buffer, _write_offset, size);
    _write_offset += ret;
    return ret;
}

int Socket::send(const uint8_t *buffer, size_t size) {
    const int ret = _driver.send(_sockfd, buffer, _write_offset, size);
    _write_offset = 0;
    return ret;
}

void Socket::send() {
    if (_write_offset == 0) {
        return;
    }
    _driver.send(_sockfd);
    _write_offset = 0;
}

bool Socket::phy_link_up() { return _driver.link_up(); }

uint16_t Socket::rx_byte_count() { return _driver.get_rx_byte_count(_sockfd); }

} // namespace W5500
