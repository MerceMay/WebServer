#pragma once
#include <vector>
#include <string>
#include <algorithm>

// A simple buffer class that manages memory more efficiently than std::string
// for frequent read/write operations
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    void append(const std::string& str)
    {
        append(str.c_str(), str.size());
    }

    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }

    void unwrite(size_t len)
    {
        writerIndex_ -= len;
    }

    // Find a pattern in the buffer
    const char* find(const char* target, size_t targetLen) const
    {
        const char* found = std::search(peek(), beginWrite(), target, target + targetLen);
        return found == beginWrite() ? nullptr : found;
    }

    // Find CRLF in buffer
    const char* findCRLF() const
    {
        const char crlf[] = "\r\n";
        return find(crlf, 2);
    }

    // Reset buffer to initial state and shrink if too large
    void shrink(size_t reserve = 0)
    {
        retrieveAll();
        buffer_.resize(kCheapPrepend + reserve);
        buffer_.shrink_to_fit();
    }

    size_t size() const { return buffer_.size(); }
    size_t capacity() const { return buffer_.capacity(); }

    // For compatibility with existing string-based code
    std::string toString() const
    {
        return std::string(peek(), readableBytes());
    }

    void clear()
    {
        retrieveAll();
    }

private:
    char* begin()
    {
        return &*buffer_.begin();
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            // grow the buffer
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                     begin() + writerIndex_,
                     begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};