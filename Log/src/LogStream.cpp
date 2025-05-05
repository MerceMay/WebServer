#include "LogStream.h"
#include <algorithm>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10); // get the last digit
        i /= 10;                            // remove the last digit
        *p++ = zero[lsd];                   // get the corresponding character for the last digit
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}

template <typename T>
void LogStream::formatInteger(T val)
{
    if (buffer_.available() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), val);
        buffer_.add(len);
    }
}

LogStream::LogStream(LogStream&& other) noexcept
    : buffer_(std::move(other.buffer_)) {}

LogStream& LogStream::operator=(LogStream&& other) noexcept
{
    if (this != &other)
    {
        buffer_ = std::move(other.buffer_);
    }
    return *this;
}

LogStream& LogStream::operator<<(bool val)
{
    buffer_.append(val ? "1" : "0", 1);
    return *this;
}
LogStream& LogStream::operator<<(short val)
{
    *this << static_cast<int>(val);
    return *this;
}
LogStream& LogStream::operator<<(unsigned short val)
{
    *this << static_cast<unsigned int>(val);
    return *this;
}
LogStream& LogStream::operator<<(int val)
{
    formatInteger(val);
    return *this;
}
LogStream& LogStream::operator<<(unsigned int val)
{
    formatInteger(val);
    return *this;
}
LogStream& LogStream::operator<<(long val)
{
    formatInteger(val);
    return *this;
}
LogStream& LogStream::operator<<(unsigned long val)
{
    formatInteger(val);
    return *this;
}
LogStream& LogStream::operator<<(long long val)
{
    formatInteger(val);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long val)
{
    formatInteger(val);
    return *this;
}

LogStream& LogStream::operator<<(float val)
{
    *this << static_cast<double>(val);
    return *this;
}
LogStream& LogStream::operator<<(double val)
{
    if (buffer_.available() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", val); // the maximum number of significant digits is 12
        buffer_.add(len);
    }
    return *this;
}
LogStream& LogStream::operator<<(long double val)
{
    if (buffer_.available() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", val); // the maximum number of significant digits is 12
        buffer_.add(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(char val)
{
    buffer_.append(&val, 1);
    return *this;
}
LogStream& LogStream::operator<<(const char* val)
{
    const char* str = reinterpret_cast<const char*>(val);
    if (str)
    {
        buffer_.append(str, std::strlen(str));
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}
LogStream& LogStream::operator<<(const void* val)
{
    if (val)
    {
        char buf[2 + sizeof(void*) * 2 + 1]; // 2 for "0x", sizeof(void*) * 2 for hex digits, 1 for '\0'
        int len = snprintf(buf, sizeof(buf), "%p", val);
        buffer_.append(buf, len);
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}
LogStream& LogStream::operator<<(const unsigned char* val)
{
    if (val)
    {
        buffer_.append(reinterpret_cast<const char*>(val), std::strlen(reinterpret_cast<const char*>(val)));
    }
    else
    {
        buffer_.append("(null)", 6);
    }
    return *this;
}

LogStream& LogStream::operator<<(const std::string& val)
{
    buffer_.append(val.c_str(), val.size());
    return *this;
}
LogStream& LogStream::operator<<(const Buffer& val)
{
    buffer_.append(val.data(), val.size());
    return *this;
}