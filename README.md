## W5500 driver

This is a driver for the
[W5500 ethernet IC](https://www.wiznet.io/product-item/w5500/),
targeted at embedded applications. It aims
to strike a balance between performance and readability / modern C++ style, as
well as easy portability through a `Bus` interface, which should be the only
code necessary to start using this library on your platform of choice. A
partially complete bus implementation that works with the
[opencm3](https://github.com/libopencm3/libopencm3)
library for STM32 arm cores can be seen in the
[Buses](/include/W5500/Buses)
folder for inspiration.

### Usage

To use this library, you will first need to extend / implement a `Bus` to tie
the driver to your system. As an example, here is an excerpt of a project that
uses this driver on an STM32F0 series MCU. The default bus header provides SPI
implementation, but here we also tie the logging & realtime clock functionality
to our particular system:

```c++
static const uint32_t *STM32F0_CHIP_ID = reinterpret_cast<uint32_t *>(0x1FFFF7AC);

class EtherBus : public W5500::Buses::OpenCM3 {
    // Inherit constructor
    using W5500::Buses::OpenCM3::OpenCM3;

    void init() {
        // Super init
        W5500::Buses::OpenCM3::init();

        // Initialize LFSR (for PRNG) using unique ID of this chip
        uint64_t *option_data = reinterpret_cast<uint64_t*>(STM32F0_CHIP_ID);
        _lfsr_state = *option_data;
    }

    // Realtime isn't baked into opencm3, so need to implement that virtual
    // fun here. Substitute in your own monotonic time implementation.
    uint64_t millis() override {
        return Realtime::millis();
    }

    // Proxy logging from network stack to uart.
    // Again, you need to provide your own implementation here.
    void log(const char *msg, ...) override {
        va_list args;
        va_start(args, msg);
        Uart::va_log(msg, args);
        va_end(args);
    }
};
```

Now that we've created a bus interface for our system, we can use it to create
an instance of the driver and some sockets:

```c++
// Create an instance of the bus, passing in the relevant IOs
EtherBus _w5500_bus{SPI1, GPIOA, GPIO4};

// Use that bus to create a drvier
W5500::W5500 _tcpip{_w5500_bus};

// We'll use socket 1 for DHCP
W5500::UdpSocket _dhcp_socket{_tcpip, 1};
W5500::Protocols::DHCP::Client _dhcp_client{_tcpip, _dhcp_socket, "Test Controller"};

// And socket 2 for NTP
W5500::UdpSocket _ntp_socket{_tcpip, 2};
W5500::Protocols::NTP::Client _ntp_client{_tcpip, _ntp_socket, 132, 163, 96, 1};
```

In our application startup, we can then initialize everything like so:

```c++
// Initialize the bus
_driver.init();

// Ensure the IC has been reset to an initial state
_driver.reset();

// Since we're using a DHCP client, let's also set a hostname for this device
snprintf(controller_dhcp_id, sizeof(controller_dhcp_id), "Controller-%08lx%08lx%08lx",
    STM32F0_CHIP_ID[0], STM32F0_CHIP_ID[1], STM32F0_CHIP_ID[2]);

// We also need a MAC address. Since our PRNG is seeded by the chip ID, this
// should be a unique value but stable across restarts.
uint8_t mac[6];
for (int i = 0; i < 6; i++) {
    mac[i] = _w5500_bus.random() & 0xFF;
}
_driver.set_mac(mac);
```

For provided protocols such as DHCP, you should be sure to call their `update()`
method relatively frequently. In general, placing them in your top level
application loop is a good idea. Protocols are implemented such that if there
is no work to be done, the `update()` call will be very short.

For dealing with persistent (TCP) connections, you will want to periodically
check that the link is still up, so that you can reconnect on error. An example
implementation might look something like this:

```c++
void check_connection(Controller& controller) {
    // Read the physical link state
    if (!_socket.phy_link_up()) {
        // If the link is down, there's nothing to be done.
        if (_socket.connected()) {
            // If it still thinks it's connected, close it.
            _socket.close();
            Uart::log("Physical link down, closing socket\n");
        }
        return;
    }

    // Read the interrupt flags for the socket
    auto flags = _socket.get_interrupt_flags();

    // Socket has just connected - handle our connection callback
    if (flags & W5500::Registers::Socket::InterruptFlags::CONNECT) {
        Uart::log("Socket established\n");
        _socket.clear_interrupt_flag(W5500::Registers::Socket::InterruptFlags::CONNECT);

        // Handle application level logic on connection
        controller.handle_connect();
    }

    // Disconnect - close and try again
    if (flags & W5500::Registers::Socket::InterruptFlags::DISCONNECT) {
        Uart::log("Socket disconnected! Closing.\n");
        _socket.close();
        _socket.clear_interrupt_flag(W5500::Registers::Socket::InterruptFlags::DISCONNECT);
    }

    // Timeout - reset socket and try again
    if (flags & W5500::Registers::Socket::InterruptFlags::TIMEOUT) {
        Uart::log("Socket timed out - closing\n");
        _socket.close();
        _socket.clear_interrupt_flag(W5500::Registers::Socket::InterruptFlags::TIMEOUT);
    }

    // Ensure socket is connected
    if (!_socket.connected()) {
        if (!_socket.ready()) {
            if (!_socket.init()) {
                Uart::log("Failed to initialize socket!\n");
                _socket.close();
                return;
            }
            Uart::log("Initialized socket\n");
        }
        if (Realtime::millis() - _last_connect_attempt > _connect_backoff_ms
                && !_socket.connecting()) {
            _last_connect_attempt = Realtime::millis();
            Uart::log("Reconnecting to %u.%u.%u.%u:%u\n",
                _ip[0], _ip[1], _ip[2], _ip[3],
                _port);
            _socket.connect(_ip, _port);
        }
    }
}
```

For sending and receiving data, use the peek/read and write/send methods on
`Socket`. `peek` will return data without advancing the on-chip read pointer,
whereas `read` will read data and then advance the read pointer so that the IC
can receive more data off the network. Likewise, the `write` command will buffer
data to the IC, but will not advance the write pointer and trigger a network
operation. This is more useful for UDP connections, where the `send` command
terminates the current packet, than for streaming connections.
As an example, here is how one might implement an echo client:

```c++
void echo_loop() {
    // Check how many bytes have been received off the wire
    const auto rx_byte_count = _socket.rx_byte_count();

    // If there's no data to read, return early.
    if (rx_byte_count == 0)  {
        return;
    }

    // If there is data to read, copy it to a local buffer
    // For a more robust implementation, one would chunk the reads to
    // fit in available memory.
    uint8_t data[rx_byte_count];
    _socket.read(data, rx_byte_count);

    // Send the data back out over the socket. Again, chunking would make
    // sense for a real application.
    _socket.send(data, rx_byte_count);
}
```
