#pragma once

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Timer.h"
#include "Util.h"
#include "Buffer.h"
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>

constexpr int DEFAULT_EXPIRED_TIME = 3000;
constexpr int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // the default keep-alive time is 5 minutes
constexpr size_t MAX_BUFFER_SIZE = 64 * 1024; // 64KB max buffer size to prevent memory exhaustion

class EventLoop;
class Channel;
class TimerNode;

class HttpData : public std::enable_shared_from_this<HttpData>
{
private:
    enum class ConnectionState
    {
        CONNECTED,
        DISCONNECTED,
        DISCONNECTING
    };

    enum class HttpMethod
    {
        POST,
        GET,
        HEAD
    };

    enum class HttpVersion
    {
        HTTP_10,
        HTTP_11
    };

    enum class ProcessState
    {
        PARSE_URL,
        PARSE_HEADERS,
        RECV_BODY,
        ANALYZE,
        FINISH
    };

    enum class ParseState
    {
        START,
        KEY,
        COLON,
        SPACE,
        VALUE,
        CR,
        LF,
        CRLF,
        BODY,
        END
    };

    enum class URLState
    {
        AGAIN,
        ERROR,
        SUCCESS,
        PATHUNINVALID
    };

    enum class HeaderState
    {
        AGAIN,
        ERROR,
        SUCCESS
    };

    enum class AnalyzeState
    {
        ERROR,
        SUCCESS
    };

    const std::unordered_map<std::string, std::string> mime = {
        {".html", "text/html"},
        {".avi", "video/x-msvideo"},
        {".mp4", "video/mp4"},
        {".bmp", "image/bmp"},
        {".c", "text/plain"},
        {".cpp", "text/plain"},
        {".doc", "application/msword"},
        {".gif", "image/gif"},
        {".gz", "application/x-gzip"},
        {".htm", "text/html"},
        {".ico", "image/x-icon"},
        {".jpg", "image/jpeg"},
        {".png", "image/png"},
        {".txt", "text/plain"},
        {".mp3", "audio/mp3"},
        {"default", "text/html"}};
    std::string getMime(const std::string& suffix)
    {
        auto it = mime.find(suffix);
        if (it != mime.end())
        {
            return it->second;
        }
        else
        {
            return mime.at("default");
        }
    }

    const std::string ROOT_DIR = std::filesystem::current_path().string() + "/Resource"; // set resource directory

private:
    std::shared_ptr<EventLoop> loop_;  // the event loop which manages this HttpData
    std::shared_ptr<Channel> channel_; // the channel of this HttpData
    std::weak_ptr<TimerNode> timer_;   // timer_ is managed by timerManager, so it is a weak pointer

    int fd_;
    Buffer inBuffer_;                 // Input buffer - more efficient than std::string
    Buffer outBuffer_;                // Output buffer - more efficient than std::string
    bool error_;                      // Error flag
    ConnectionState connectionState_; // Connection state
    HttpMethod httpMethod_;
    HttpVersion httpVersion_;

    std::string filename_;
    std::string path_;

    int readIdx_;

    ProcessState processState_;
    ParseState parseState_;

    bool isKeepAlive_;
    std::map<std::string, std::string> headerFields_;

    std::string request_line;

private:
    void handleConnect(); // Handle connection-related logic, adjust events and timeout.

    void handleRead(); // Handle read events, read data from the socket and parse the request.

    void handleWrite(); // Handle write events, write buffer data to the socket.

private:
    URLState parseRequestLine(); // Process the request line

    bool parseHttpMethod(const std::string& request_line); // Parse the HTTP method from the request line

    bool processUrl(const std::string& url); // Process and validate the URL

    bool parseHttpVersion(const std::string& version); // Parse the HTTP version

    bool isValidAndSafePath(const std::string& path); // Check the safety of the path

    bool isDirectory(const std::string& path); // Check if the path is a directory

private:
    HeaderState parseHeader(); // Process the request headers

    std::string trimTrailingSpaces(const std::string& str); // Remove trailing spaces and tabs from a string

private:
    AnalyzeState generateSendHTTP(); // Generate the HTTP response

    void sendErrorHttp(int fd, int err_num, std::string msg); // Send an error response to the client

    std::string getFileType(const std::string& filename); // Get the file type based on the filename

    std::string buildResponseHeader(const std::string& filetype); // Build the response header

    AnalyzeState handleHelloRequest(); // Handle "hello" requests

    AnalyzeState handleIndexRequest(std::string& header); // Handle "index.html" requests (file listing)

    AnalyzeState handleFileRequest(std::string& header, const std::string& filetype); // Handle regular file requests

    AnalyzeState sendFileContent(const std::string& fullPath, const std::string& header, size_t fileSize); // Send file content

    bool curPathFileList(std::string path, std::string& body); // Handle file listing requests

private:
    void updateEventAndTimeout(std::shared_ptr<Channel> channel, unsigned int newEvent, int timeout); // Update events and expiration time

    void updateEvent(std::shared_ptr<Channel> channel, unsigned int newEvent); // Update events

public:
    HttpData(std::shared_ptr<EventLoop> loop, int fd);
    ~HttpData();

    HttpData() = default;
    HttpData(const HttpData&) = delete;
    HttpData& operator=(const HttpData&) = delete;
    HttpData(HttpData&&) = delete;
    HttpData& operator=(HttpData&&) = delete;

    void reset();
    void linkTimer(std::shared_ptr<TimerNode> mtimer);

    void unlinkTimer();
    std::shared_ptr<EventLoop> getLoop();

    std::shared_ptr<Channel> getChannel();
    
    // Optimize buffer sizes for long-lived connections
    void optimizeBuffers();

    // Handle connection close logic, clean up resources, and remove channel_ from the event loop.
    // Called when the connection is disconnected or needs to be closed.
    void handleClose();

    void newEvent();
};
