#include "FixedBuffer.h"

template <ssize_t SIZE>
FixedBuffer<SIZE>::FixedBuffer()
{
    cur_ = buffer_.data(); // initialize cur_ to the beginning of the buffer
    zero();                // zero the buffer
}

template <ssize_t SIZE>
FixedBuffer<SIZE>::FixedBuffer(FixedBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_)) // move buffer_
{
    // recalculate cur_ position in the new buffer
    ptrdiff_t offset = other.cur_ - other.buffer_.data(); // calculate offset of cur_ relative to buffer_
    cur_ = buffer_.data() + offset;

    // clear other cur_ to maintain consistency
    other.cur_ = other.buffer_.data();
}

template <ssize_t SIZE>
FixedBuffer<SIZE>& FixedBuffer<SIZE>::operator=(FixedBuffer&& other) noexcept
{
    if (this != &other)
    {
        buffer_ = std::move(other.buffer_);                   // move other buffer_ to this buffer_
        ptrdiff_t offset = other.cur_ - other.buffer_.data(); // calculate offset of cur_ relative to buffer_
        cur_ = buffer_.data() + offset;

        // set other cur_ to the beginning of its buffer_
        other.cur_ = other.buffer_.data();
    }
    return *this;
}

template <ssize_t SIZE>
const char* FixedBuffer<SIZE>::data() const
{
    return buffer_.data();
}

template <ssize_t SIZE>
ssize_t FixedBuffer<SIZE>::size() const
{
    return static_cast<ssize_t>(cur_ - buffer_.data());
}

template <ssize_t SIZE>
ssize_t FixedBuffer<SIZE>::capacity() const
{
    return SIZE;
}

template <ssize_t SIZE>
void FixedBuffer<SIZE>::reset()
{
    cur_ = buffer_.data();
}

template <ssize_t SIZE>
void FixedBuffer<SIZE>::add(size_t len)
{
    cur_ += len;
}

template <ssize_t SIZE>
void FixedBuffer<SIZE>::zero()
{
    std::fill(buffer_.begin(), buffer_.end(), 0);
}

template <ssize_t SIZE>
char* FixedBuffer<SIZE>::current()
{
    return cur_;
}

template <ssize_t SIZE>
size_t FixedBuffer<SIZE>::available() const
{
    return SIZE - size();
}

template <ssize_t SIZE>
bool FixedBuffer<SIZE>::append(const char* str, size_t len)
{
    if (cur_ + len <= buffer_.data() + SIZE)
    {
        std::copy(str, str + len, cur_);
        add(len); // update the current position
        return true;
    }
    std::cerr << "Buffer overflow: Attempted to write " << len << " bytes, but only " << available() << " bytes are available." << std::endl;
    return false; // buffer overflow
}

template <ssize_t SIZE>
bool FixedBuffer<SIZE>::append(const char* str)
{
    return append(str, std::strlen(str));
}

template <ssize_t SIZE>
void FixedBuffer<SIZE>::append(const std::string& str)
{
    append(str.c_str(), str.size());
}

template <ssize_t SIZE>
void FixedBuffer<SIZE>::append(const std::string& str, size_t len)
{
    assert(len <= str.size());
    append(str.c_str(), len);
}
