#pragma once

#include "Buffer.h"
#include <map>
#include <string>

class HttpContext
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    enum HttpMethod
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete
    };

    enum HttpVersion
    {
        kUnknown, kHttp10, kHttp11
    };

    HttpContext()
        : state_(kExpectRequestLine),
          method_(kInvalid),
          version_(kUnknown)
    {
    }

    // Returns true if parsing is done (got a full request)
    bool parseRequest(Buffer* buf, int64_t receiveTime);

    bool gotAll() const { return state_ == kGotAll; }
    void reset()
    {
        state_ = kExpectRequestLine;
        method_ = kInvalid;
        version_ = kUnknown;
        path_.clear();
        query_.clear();
        headers_.clear();
    }

    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }
    HttpMethod method() const { return method_; }
    HttpVersion version() const { return version_; }
    const std::string& getHeader(const std::string& key) const;
    
    // Setters used by parser
    void setMethod(HttpMethod m) { method_ = m; }
    void setVersion(HttpVersion v) { version_ = v; }
    void setPath(const char* start, const char* end) { path_.assign(start, end); }
    void setQuery(const char* start, const char* end) { query_.assign(start, end); }
    void addHeader(const char* start, const char* colon, const char* end);

private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_;
    HttpMethod method_;
    HttpVersion version_;
    std::string path_;
    std::string query_;
    std::map<std::string, std::string> headers_;
};
