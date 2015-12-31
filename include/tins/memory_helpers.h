#ifndef TINS_MEMORY_HELPERS_H
#define TINS_MEMORY_HELPERS_H

#include <stdint.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include "exceptions.h"
#include "ip_address.h"
#include "ipv6_address.h"
#include "hw_address.h"

namespace Tins {
namespace Memory {

inline void read_data(const uint8_t* buffer, uint8_t* output_buffer, uint32_t size) {
    std::memcpy(output_buffer, buffer, size);
}

template <typename T>
void read_value(const uint8_t* buffer, T& value) {
    std::memcpy(&value, buffer, sizeof(value));
}

inline void write_data(uint8_t* buffer, const uint8_t* ptr, uint32_t size) {
    std::memcpy(buffer, ptr, size);
}

template <typename T>
void write_value(uint8_t* buffer, const T& value) {
    std::memcpy(buffer, &value, sizeof(value));
}

class InputMemoryStream {
public:
    InputMemoryStream(const uint8_t* buffer, uint32_t total_sz)
    : buffer_(buffer), size_(total_sz) {
    }

    InputMemoryStream(const std::vector<uint8_t>& data)
    : buffer_(&data[0]), size_(data.size()) {
    }

    void skip(uint32_t size) {
        buffer_ += size;
        size_ -= size;
    }

    bool can_read(uint32_t byte_count) const {
        return TINS_LIKELY(size_ >= byte_count);
    }
 
    template <typename T>
    T read() {
        T output;
        read(output);
        return output;
    }

    template <typename T>
    void read(T& value) {
        if (!can_read(sizeof(value))) {
            throw malformed_packet();
        }
        read_value(buffer_, value);
        skip(sizeof(value));
    }

    void read(IPv4Address& address) {
        address = IPv4Address(read<uint32_t>());
    }

    void read(IPv6Address& address) {
        if (!can_read(IPv6Address::address_size)) {
            throw malformed_packet();
        }
        address = pointer();
        skip(IPv6Address::address_size);
    }

    template <size_t n>
    void read(HWAddress<n>& address) {
        if (!can_read(HWAddress<n>::address_size)) {
            throw malformed_packet();
        }
        address = pointer();
        skip(HWAddress<n>::address_size);
    }

    void read(void* output_buffer, uint32_t output_buffer_size) {
        if (!can_read(output_buffer_size)) {
            throw malformed_packet();
        }
        read_data(buffer_, (uint8_t*)output_buffer, output_buffer_size);
        skip(output_buffer_size);
    }

    const uint8_t* pointer() const {
        return buffer_;
    }

    uint32_t size() const {
        return size_;
    }

    void size(uint32_t new_size) {
        size_ = new_size;
    }

    operator bool() const {
        return size_ > 0;
    }
private:
    const uint8_t* buffer_;
    uint32_t size_;
};

class OutputMemoryStream {
public:
    OutputMemoryStream(uint8_t* buffer, uint32_t total_sz)
    : buffer_(buffer), size_(total_sz) {
    }

    void skip(uint32_t size) {
        buffer_ += size;
        size_ -= size;
    }

    template <typename T>
    void write(const T& value) {
        if (TINS_UNLIKELY(size_ < sizeof(value))) {
            throw serialization_error();
        }
        write_value(buffer_, value);
        skip(sizeof(value));
    }

    template <typename ForwardIterator>
    void write(ForwardIterator start, ForwardIterator end) {
        const uint32_t length = std::distance(start, end); 
        if (TINS_UNLIKELY(size_ < length)) {
            throw serialization_error();
        }
        std::copy(start, end, buffer_);
        skip(length);
    }

    void write(const uint8_t* ptr, uint32_t length) {
        write(ptr, ptr + length);
    }

    void write(const IPv4Address& address) {
        write(static_cast<uint32_t>(address));
    }

    void write(const IPv6Address& address) {
        write(address.begin(), address.end());
    }

    template <size_t n>
    void write(const HWAddress<n>& address) {
        write(address.begin(), address.end());
    }

    void fill(uint32_t size, uint8_t value) {
        if (TINS_UNLIKELY(size_ < size)) {
            throw serialization_error();
        }
        std::fill(buffer_, buffer_ + size, value);
        skip(size);
    }

    uint8_t* pointer() {
        return buffer_;
    }

    uint32_t size() const {
        return size_;
    }
private:
    uint8_t* buffer_;
    uint32_t size_;
};

} // Memory
} // Tins

#endif // TINS_MEMORY_HELPERS_H
