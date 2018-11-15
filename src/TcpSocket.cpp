#include <W5500/Socket.hpp>
#include <W5500/W5500.hpp>

namespace W5500 {

bool TcpSocket::init() {
    _driver.set_socket_mode(_sockfd, SocketMode::TCP);
    _driver.send_socket_command(_sockfd, Registers::Socket::CommandValue::OPEN);
    return ready();
}

bool TcpSocket::ready() {
    Registers::Socket::StatusValue status = _driver.get_socket_status(_sockfd);
    // Return true if the socket is open, connecting or connected.
    return status == Registers::Socket::StatusValue::INIT // Opened
        || status == Registers::Socket::StatusValue::LISTEN // Server mode
        || status == Registers::Socket::StatusValue::SYN_SENT // Client conn in progress
        || status == Registers::Socket::StatusValue::SYN_RECV // Server conn in progress
        || status == Registers::Socket::StatusValue::ESTABLISHED; // Client connected
}

bool TcpSocket::connected() {
    return _driver.get_socket_status(_sockfd) == Registers::Socket::StatusValue::ESTABLISHED;
}

} // namespace W5500
