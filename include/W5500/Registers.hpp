#ifndef _W5500__W5500_REGISTERS_H_
#define _W5500__W5500_REGISTERS_H_

#include <stdint.h>

namespace W5500 {

    // Bank addresses
    constexpr uint8_t COMMON_REGISTER_BANK = 0x0;

    constexpr uint8_t SOCKET_REG(uint8_t socket_n) {
        return socket_n * 4 + 1;
    }

    constexpr uint8_t SOCKET_TX_BUFFER(uint8_t socket_n) {
        return socket_n * 4 + 2;
    }

    constexpr uint8_t SOCKET_RX_BUFFER(uint8_t socket_n) {
        return socket_n * 4 + 3;
    }

    enum class SocketMode : uint8_t {
        CLOSED = 0b0000,
        TCP    = 0b0001,
        UDP    = 0b0010,
        MACRAW = 0b0100 // Socket 0 only
    };

    // Register deinitions.
    // All registers have an offset into the bank, and a size in bytes.
    // Most significant bytes are stored in lower indexes of multi-byte regs.
    class Register {
        public:
            explicit Register(uint8_t off, uint8_t sz=1) :
                offset(off), size(sz) {}
            const uint8_t offset;
            const uint8_t size;
    };

    class CommonRegister : public Register { using Register::Register; };
    class SocketRegister : public Register { using Register::Register; };

    namespace Registers {

        // Common register block
        namespace Common {
            static CommonRegister Mode{0x00};
            static CommonRegister GatewayAddress{0x01, 4};
            static CommonRegister SubnetMaskAddress{0x05, 4};
            static CommonRegister SourceHardwareAddress{0x09, 6};
            static CommonRegister SourceIpAddress{0x0F, 4};
            static CommonRegister InterruptLevel{0x13, 2};
            static CommonRegister Interrupt{0x15};
            static CommonRegister InterruptMask{0x16};
            static CommonRegister SocketInterrupt{0x17};
            static CommonRegister SocketInterruptMask{0x18};
            static CommonRegister RetryTime{0x19, 2};
            static CommonRegister RetryCount{0x1B};
            static CommonRegister PPPLCPRequestTimer{0x1C};
            static CommonRegister PPPLCPRequestMagic{0x1D};
            static CommonRegister PPPDestinationMAC{0x1E, 6};
            static CommonRegister PPPSessionId{0x24, 2};
            static CommonRegister PPPMaxSegmentSize{0x26, 2};
            static CommonRegister UnreachableIP{0x28, 4};
            static CommonRegister UnreachablePort{0x2C, 2};
            static CommonRegister PhyConfig{0x2E};
            static CommonRegister ChipVersion{0x39};

            enum class ModeFlags : uint8_t {
                // If set to 1, registers will be initialized.
                // Cleared to 0 after SW reset.
                RESET = (1 << 7),
                // 0: Disable. 1: Eanble.
                // If enabled and magic packet is received and processed,
                // an interrupt will be triggered.
                WAKE_ON_LAN = (1 << 5),
                // If set, block response to pings
                PING_BLOCK = (1 << 4),
                // 0: PPoE Disabled. 1: PPoE enabled.
                PPOE_MODE = (1 << 3),
                // 0: Disable, 1: Enable.
                // If enabled, force ARP whenever data is sent.
                FORCE_ARP = (1 << 1)
            };

            enum class InterruptFlags : uint8_t {
                // Set as 1 when own source IP matches sender of a received ARP
                IP_CONFLICT = (1 << 7),
                // Set as 1 when ICMP Unreachable packet received.
                // Relevant Port and IP are in UIPR and UPORTR
                UNREACHABLE = (1 << 6),
                // PPoE Connection CLosed
                PPOE_CLOSED = (1 << 5),
                // Magic packet received
                MAGIC_PACKET = (1 << 4)
            };

            class InterruptRegisterValue {
                public:
                    InterruptRegisterValue(uint8_t flags) : _flags(flags) {}
                    bool operator &(InterruptFlags flag) {
                        return _flags & static_cast<uint8_t>(flag);
                    }

                private:
                    const uint8_t _flags;
            };

            enum class InterruptMaskFlags : uint8_t {
                // Masked (0) interrupts will not trigger.
                IP_CONFLICT = (1 << 7),
                UNREACHABLE = (1 << 6),
                PPOE_CLOSED = (1 << 5),
                MAGIC_PACKET = (1 << 4)
            };

            enum class PhyOperationMode : uint8_t {
                BASE10_HALF_DUPLEX = 0b000,
                BASE10_FULL_DUPLEX = 0b001,
                BASE100_HALF_DUPLEX = 0b010,
                BASE100_FULL_DUPLEX = 0b011,
                BASE100_HALF_DUPLEX_AUTONEGOTIATE = 0b100,
                POWER_DOWN = 0b110,
                AUTO_ALL = 0b111
            };

            enum class PhyConfigFlags : uint8_t {
                // When 0 written, PHY is reset. After reset will be 1.
                RESET = (1 << 7),
                // Operation Mode. If 0, mode configured using hardware pins.
                // If 1, configured using bits [5:3] of this register.
                OPERATION_MODE = (1 << 6),

                // Read-only status bits
                DUPLEX_STATUS = (1 << 2),
                SPEED_STATUS = (1 << 1),
                LINK_STATUS = (1 << 0),
            };
        }

        // Socket register block
        namespace Socket {
            static SocketRegister Mode{0x0};
            static SocketRegister Command{0x01};
            static SocketRegister Interrupt{0x02};
            static SocketRegister Status{0x03};
            static SocketRegister SourcePort{0x04, 2};
            static SocketRegister DestHardwareAddress{0x06, 6};
            static SocketRegister DestIPAddress{0x0c, 4};
            static SocketRegister DestPort{0x10, 2};
            static SocketRegister MaxSegmentSize{0x12, 2};
            static SocketRegister IP_TOS{0x15};
            static SocketRegister IP_TTL{0x16};
            static SocketRegister RxBufferSize{0x1E};
            static SocketRegister TxBufferSize{0x1F};
            static SocketRegister TxFreeSize{0x20, 2};
            static SocketRegister TxReadPointer{0x22, 2};
            static SocketRegister TxWritePointer{0x24, 2};
            static SocketRegister RxReceivedSize{0x26, 2};
            static SocketRegister RxReadPointer{0x28, 2};
            static SocketRegister RxWritePointer{0x2A, 2};
            static SocketRegister InterruptMask{0x2C};
            static SocketRegister FragmentOffset{0x2D, 2};
            static SocketRegister KeepAliveTimer{0x2F};

            enum class ModeFlags : uint8_t {
                // In UDP mode, 0 disables / 1 enables multicast
                // In MACRAW mode, a 0 value allows the W5500 to receive all
                // ethernet packets. When set to 1, can only receive broadcasts
                // or directly addressed packets.
                MULTI_MFEN = (1 << 7),
                // Broadcast blocking
                // If set, disable receipt of broadcast messages in UDP mode.
                BROADCAST_BLOCK = (1 << 6),
                // In TCP mode:
                // If set, enable 'No Delayed ACK' TCP option.
                // In UDP mode:
                // If set, use IGMP version 1. If cleared, IGMP version 2
                // In MACRAW mode:
                // 0: Disable multicast blocking, 1: Enable multicast blocking
                ND_MC_MMC = (1 << 5),
                // UDP Mode:
                // 0: Disable unicast blocking
                // 1: Enable unicast blocking
                // MACRAW mode:
                // 0: Disable IPV6 blocking
                // 1: Enable IPV6 blocking
                UCASTB_MIP6B = (1 << 4),
            };

            enum class CommandValue : uint8_t {
                // Socket is initialized and opened according to protocol
                // selected in Sn_MR[3:0]
                OPEN = 0x01,
                // Only valid in TCP mode.
                // Socket acts as TCP server and waits for SYN.
                // Sn_SR changes from SOCK_INIT to SOCK_LISTEN.
                // When connection is received, Sn_SR becomes SOCK_ESTABLISHED
                // and SnIR[0] becomes 1.
                // When connection fails, Sn_IR[3] becomes 1 and Sn_SR becomes
                // SOCK_CLOSED.
                LISTEN = 0x02,
                // Only valid in TCP mode.
                // Trigger connection to Sn_DIPR on port Sn_DPORT.
                // On success, Sn_SR becomes SOCK_ESTABLISHED and Sn_IR[0]
                // becomes 1.
                CONNECT = 0x04,
                // Only valid in TCP mode.
                // Disconnect either a client or server connection.
                // When disconnect is complete, Sn_SR changes to SOCK_CLOSED.
                DISCONNECT = 0x08,
                // Force close socket
                CLOSE = 0x10,
                // Transmit all data in the socket TX buffer.
                SEND = 0x20,
                // Only valid in UDP mode.
                // Send data using target from Sn_DHAR, not ARP request.
                SEND_MAC = 0x21,
                // Only valid in TCP mode.
                // Check connection status by sending 1 byte keep-alive packet.
                // If no response is received within the timeout interval,
                // the connection is terminated.
                SEND_KEEPALIVE = 0x22,
                // Complete the processing of received data in RX buffer using
                // read pointer register.
                RECV = 0x40
            };

            enum class InterruptFlags : uint8_t {
                SEND_OK = (1 << 4),
                TIMEOUT = (1 << 3),
                RECV = (1 << 2),
                DISCONNECT = (1 << 1),
                CONNECT = (1 << 0)
            };

            class InterruptRegisterValue {
                public:
                    InterruptRegisterValue(uint8_t flags) : _flags(flags) {}
                    bool operator &(InterruptFlags flag) {
                        return _flags & static_cast<uint8_t>(flag);
                    }

                private:
                    const uint8_t _flags;
            };

            enum class InterruptMaskFlags : uint8_t {
                SEND_OK = (1 << 4),
                TIMEOUT = (1 << 3),
                RECV = (1 << 2),
                DISCONNECT = (1 << 1),
                CONNECT = (1 << 0)
            };

            enum class StatusValue : uint8_t {
                // Socket is released.
                CLOSED = 0x00,
                // Socket opened in TCP mode
                INIT = 0x13,
                // Socket in TCP server mode, awaiting connection
                LISTEN = 0x14,
                // SYN sent, awaiting SYN-ACK/NAK
                SYN_SENT = 0x15,
                // SYN received in server mode, awaiting ACK
                SYN_RECV = 0x16,
                // TCP connection established
                ESTABLISHED = 0x17,
                // Socket in process of closing
                FIN_WAIT = 0x18,
                CLOSING = 0x1A,
                TIME_WAIT = 0x1B,
                // Socket has received FIN and is closing.
                CLOSE_WAIT = 0x1C,
                // Waiting for final ACK of FIN packet
                LAST_ACK = 0x1D,
                // Socket is in UDP mode
                UDP = 0x22,
                // Socket is in MACRAW mode
                MACRAW = 0x42
            };

            enum class BufferSize : uint8_t {
                SZ_ZERO = 0,
                SZ_1K = 0x01,
                SZ_2K = 0x02,
                SZ_4K = 0x04,
                SZ_8K = 0x08,
                SZ_16K = 0x10
            };
        }
    }
}

#endif // #ifndef _W5500__W5500_REGISTERS_H_
