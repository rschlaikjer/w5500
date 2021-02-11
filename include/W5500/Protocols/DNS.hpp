#ifndef _W5500__W5500_PROTOCOLS_DNS_H_
#define _W5500__W5500_PROTOCOLS_DNS_H_

#include <stdint.h>

#include <W5500/Utility/CRC16.hpp>
#include <W5500/W5500.hpp>

namespace W5500 {
namespace Protocols {
namespace DNS {

static const uint16_t port = 53;

class DNSCacheEntry {
  public:
    bool is_filled(uint64_t now_ms) {
        // If we have not been set, return false
        if (!_filled) {
            return false;
        }

        // If we've been set but the TTL has expired, return false
        if (_expires_at < now_ms) {
            _filled = false;
            return false;
        }

        // Otherwise, we hold a valid IP
        return true;
        ;
    }

    void clear() {
        // Clear the filled flag
        _filled = false;
    }

    void get(uint8_t out[4]) {
        out[0] = _ip[0];
        out[1] = _ip[1];
        out[2] = _ip[2];
        out[3] = _ip[3];
    }

    uint64_t expires_at() { return _expires_at; }

    uint16_t query_id() { return _query_id; }

    void set(uint16_t query_id, uint8_t ip[4], uint64_t expires_at) {
        _filled = true;
        _query_id = query_id;
        _expires_at = expires_at;
        _ip[0] = ip[0];
        _ip[1] = ip[1];
        _ip[2] = ip[2];
        _ip[3] = ip[3];
    }

  private:
    bool _filled = false;
    uint64_t _expires_at = 0;
    uint16_t _query_id = 0;
    uint8_t _ip[4];
};

class DNSCache {
    static const int size = 8;

  public:
    bool get(uint64_t now_ms, uint16_t query_id, uint8_t ip[4]) {
        // Check to see if this query ID matches any of our cached results
        for (int i = 0; i < size; i++) {
            if (_entries[i].is_filled(now_ms) &&
                _entries[i].query_id() == query_id) {
                _entries[i].get(ip);
                return true;
            }
        }
        return false;
    }

    void store(uint16_t query_id, uint8_t ip[4], uint64_t now_ms,
               uint64_t expires_at) {
        // Iterate over our cache entries, and see if there is an
        // unfilled slot. If there is, fill it. If there is not, expire
        // the slot that would have TTLd next anyway
        int candidate = 0;
        uint64_t candidate_expiry_time_ms = 0XFFFFFFFFFFFFFFFF;
        for (int i = 0; i < size; i++) {
            if (!_entries[i].is_filled(now_ms)) {
                _entries[i].set(query_id, ip, expires_at);
                return;
            } else if (_entries[i].expires_at() < candidate_expiry_time_ms) {
                candidate = i;
                candidate_expiry_time_ms = _entries[i].expires_at();
            }
        }

        // If we got here, there were no open slots. Replace our
        // candidate slot with the new value.
        _entries[candidate].set(query_id, ip, expires_at);
    }

    bool has_entry(uint64_t now_ms, uint16_t query_id) {
        for (int i = 0; i < size; i++) {
            if (_entries[i].is_filled(now_ms) &&
                _entries[i].query_id() == query_id) {
                return true;
            }
        }
        return false;
    }

  private:
    DNSCacheEntry _entries[size];
};

class Client {
  public:
    Client(W5500 &driver, UdpSocket &socket, DNSCache &cache, uint8_t a = 0,
           uint8_t b = 0, uint8_t c = 0, uint8_t d = 0)
        : _driver(driver), _socket(socket), _cache(cache) {
        _ip[0] = a;
        _ip[1] = b;
        _ip[2] = c;
        _ip[3] = d;
    }

    void update();
    bool query(const char *hostname, uint16_t *query_id_out);
    bool get(uint16_t query_id, uint8_t ip[4]);
    void set_server_ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

  private:
    W5500 &_driver;
    UdpSocket &_socket;
    DNSCache &_cache;

    // DNS server IP
    uint8_t _ip[4];

    void encode_short(uint8_t *buf, uint16_t v);
    uint16_t decode_short(uint8_t *buf);
    uint32_t decode_int(uint8_t *buf);
    bool parse_packet();
    uint16_t next_query_id();
};

} // namespace DNS
} // namespace Protocols
} // namespace W5500

#endif // #ifndef _W5500__W5500_PROTOCOLS_DNS_H_
