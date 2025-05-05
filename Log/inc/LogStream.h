#pragma once
#include "FixedBuffer.h"
#include <cstring>
class LogStream
{
public:
    using Buffer = FixedBuffer<kSmallBuffer>;

private:
    Buffer buffer_; // 4K
    template <typename T>
    void formatInteger(T val);
    static const int kMaxNumericSize = 64;

public:
    LogStream() = default;
    ~LogStream() = default;
    LogStream(const LogStream&) = delete;            // Disable copy constructor
    LogStream& operator=(const LogStream&) = delete; // Disable copy assignment
    LogStream(LogStream&& other) noexcept;
    LogStream& operator=(LogStream&& other) noexcept;

    void append(const char* logline, size_t len) { buffer_.append(logline, len); }
    const char* data() const { return buffer_.data(); }
    size_t length() const { return buffer_.size(); }
    void reset() { buffer_.reset(); }
    void zero() { buffer_.zero(); }
    void append(const std::string& str) { buffer_.append(str.c_str(), str.size()); }
    const Buffer& buffer() const { return buffer_; }

    LogStream& operator<<(bool);
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);
    LogStream& operator<<(const Buffer&);
};