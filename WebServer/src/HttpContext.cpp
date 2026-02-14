#include "HttpContext.h"
#include <algorithm>

bool HttpContext::parseRequest(Buffer* buf, int64_t receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            const char* crlf = std::search(buf->peek(), buf->beginWrite(), "\r\n", "\r\n" + 2);
            if (crlf < buf->beginWrite())
            {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    buf->retrieve(crlf + 2 - buf->peek());
                    state_ = kExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            const char* crlf = std::search(buf->peek(), buf->beginWrite(), "\r\n", "\r\n" + 2);
            if (crlf < buf->beginWrite())
            {
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    addHeader(buf->peek(), colon, crlf);
                }
                else
                {
                    // Empty line, end of headers
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieve(crlf + 2 - buf->peek());
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectBody)
        {
            // TODO: Handle body (for POST)
            state_ = kGotAll;
            hasMore = false;
        }
    }
    return ok;
}

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if (space != end && method_ == kInvalid)
    {
        std::string m(start, space);
        if (m == "GET") method_ = kGet;
        else if (m == "POST") method_ = kPost;
        else if (m == "HEAD") method_ = kHead;
        else method_ = kInvalid;
        
        if (method_ != kInvalid)
        {
            start = space + 1;
            space = std::find(start, end, ' ');
            if (space != end)
            {
                const char* question = std::find(start, space, '?');
                if (question != space)
                {
                    setPath(start, question);
                    setQuery(question + 1, space);
                }
                else
                {
                    setPath(start, space);
                }
                
                start = space + 1;
                std::string v(start, end);
                if (v == "HTTP/1.0") version_ = kHttp10;
                else if (v == "HTTP/1.1") version_ = kHttp11;
                else version_ = kUnknown;
                
                succeed = (version_ != kUnknown);
            }
        }
    }
    return succeed;
}

void HttpContext::addHeader(const char* start, const char* colon, const char* end)
{
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size()-1]))
    {
        value.resize(value.size()-1);
    }
    headers_[field] = value;
}

const std::string& HttpContext::getHeader(const std::string& key) const
{
    auto it = headers_.find(key);
    if (it != headers_.end())
    {
        return it->second;
    }
    static const std::string empty;
    return empty;
}
