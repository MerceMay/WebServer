#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

constexpr ssize_t kSmallBuffer = 4096;        // 4K
constexpr ssize_t kLargeBuffer = 4096 * 1000; // 4M

template <ssize_t SIZE>
class FixedBuffer
{
private:
    std::array<char, SIZE> buffer_; // use std::array to manage the buffer
    char* cur_;                     // current position in the buffer

public:
    FixedBuffer();
    ~FixedBuffer() = default;

    FixedBuffer(const FixedBuffer&) = delete;            // prevent copy constructor
    FixedBuffer& operator=(const FixedBuffer&) = delete; // prevent copy assignment

    FixedBuffer(FixedBuffer&& other) noexcept;

    FixedBuffer& operator=(FixedBuffer&& other) noexcept;

    const char* data() const;                        // return the beginning of the buffer
    ssize_t size() const;                            // return the size of the buffer
    ssize_t capacity() const;                        // return the capacity of the buffer
    void reset();                                    // reset the buffer
    void add(size_t len);                            // add length to the buffer
    void zero();                                     // zero the buffer
    char* current();                                 // return the current position in the buffer
    size_t available() const;                        // return the available space in the buffer
    bool append(const char* str, size_t len);        // append C string with length
    bool append(const char* str);                    // append C string
    void append(const std::string& str);             // append std::string
    void append(const std::string& str, size_t len); // append std::string with specified length
};

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;