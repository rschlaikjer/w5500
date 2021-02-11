#include <W5500/Socket.hpp>
#include <W5500/W5500.hpp>

namespace W5500 {

const uint16_t udp_header_size = 8;

bool UdpSocket::init() {
    _driver.set_socket_mode(_sockfd, SocketMode::UDP);
    _driver.send_socket_command(_sockfd, Registers::Socket::CommandValue::OPEN);
    return ready();
}

bool UdpSocket::ready() {
    Registers::Socket::StatusValue status = _driver.get_socket_status(_sockfd);
    return status == Registers::Socket::StatusValue::UDP;
}

bool UdpSocket::has_packet() {
    // Get the pending byte count
    const uint16_t rx_bytes = _driver.get_rx_byte_count(_sockfd);

    // If it's less than the fixed size header, not a packet
    return rx_bytes >= udp_header_size;
}

int UdpSocket::peek_packet(uint8_t source_ip[4], uint16_t &source_port) {
    // If there's no data, return -1
    if (!has_packet()) {
        return -1;
    }

    // Peek the packet header
    uint8_t buf[udp_header_size];
    _driver.peek(_sockfd, buf, udp_header_size);

    // Copy the IP to the output buffer
    memcpy(source_ip, buf, 4);

    // Set the source port
    source_port = (buf[4] << 8 | buf[5]);

    // Return the number of bytes in the packet
    return (buf[6] << 8 | buf[7]);
}

int UdpSocket::read_packet_header(uint8_t source_ip[4], uint16_t &source_port) {
    // If there's no data, return -1
    if (!has_packet()) {
        return -1;
    }

    // Read the packet header.
    // Use Socket method so we don't mess with the _packet_bytes_remaining
    uint8_t buf[udp_header_size];
    Socket::read(buf, udp_header_size);

    // Copy the IP to the output buffer
    memcpy(source_ip, buf, 4);

    // Set the source port
    source_port = (buf[4] << 8 | buf[5]);

    // Return the number of bytes in the packet
    _packet_bytes_remaining = (buf[6] << 8 | buf[7]);
    return _packet_bytes_remaining;
}

uint8_t UdpSocket::read() {
    uint8_t val = Socket::read();
    _packet_bytes_remaining--;
    return val;
}

int UdpSocket::read(uint8_t *buffer, size_t size) {
    int read = Socket::read(buffer, size);
    _packet_bytes_remaining -= read;
    return read;
}

void UdpSocket::flush() {
    Socket::flush();
    _packet_bytes_remaining = 0;
}

int UdpSocket::remaining_bytes_in_packet() { return _packet_bytes_remaining; }

void UdpSocket::skip_to_packet_end() { read(nullptr, _packet_bytes_remaining); }

} // namespace W5500
