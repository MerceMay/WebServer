#include "HttpData.h"

HttpData::URLState HttpData::parseRequestLine()
{
    // Step 1: find the end of the request line
    size_t pos = inBuffer_.find("\r\n", readIdx_);
    if (pos == std::string::npos)
    {
        return URLState::AGAIN; // if the request line is not complete, wait next event to read more data
    }

    // Step 2: extract the request line
    request_line = inBuffer_.substr(readIdx_, pos - readIdx_);
    readIdx_ = pos + 2; // update read index to the next line, past \r\n

    // Step 3: get the HTTP method
    if (!parseHttpMethod(request_line))
    {
        return URLState::ERROR; // not a valid HTTP method
    }

    // Step 4: get the URL
    size_t url_start = request_line.find(' ') + 1;
    size_t url_end = request_line.find(' ', url_start);
    if (url_end == std::string::npos)
    {
        return URLState::ERROR;
    }

    std::string url = request_line.substr(url_start, url_end - url_start);
    if (!processUrl(url))
    {
        return URLState::PATHUNINVALID;
    }

    // Step 5: get the HTTP version
    std::string version = request_line.substr(url_end + 1);
    if (!parseHttpVersion(version))
    {
        return URLState::ERROR; // not a valid HTTP version
    }

    return URLState::SUCCESS;
}

bool HttpData::parseHttpMethod(const std::string& request_line)
{
    size_t method_end = request_line.find(' ');
    if (method_end == std::string::npos)
    {
        return false;
    }

    std::string method = request_line.substr(0, method_end);
    if (method == "GET")
    {
        httpMethod_ = HttpMethod::GET;
    }
    else if (method == "POST")
    {
        httpMethod_ = HttpMethod::POST;
    }
    else if (method == "HEAD")
    {
        httpMethod_ = HttpMethod::HEAD;
    }
    else
    {
        return false; // not a valid HTTP method
    }

    return true;
}

bool HttpData::processUrl(const std::string& url)
{
    // Step 1: set default values as index.html and ROOT_DIR
    if (url.empty() || url == "/")
    {
        filename_ = "index.html";
        path_ = ROOT_DIR;
        return true;
    }

    // Step 2: trim the URL to remove query parameters
    size_t query_pos = url.find('?');
    std::string clean_url = (query_pos != std::string::npos) ? url.substr(0, query_pos) : url;

    // Step 3: parse the URL to get the path and filename
    size_t last_slash = clean_url.find_last_of('/');
    if (last_slash == std::string::npos)
    {
        // there is no slash, set path to ROOT_DIR and filename to the URL
        path_ = ROOT_DIR;
        filename_ = clean_url;
    }
    else
    {
        // there is a slash, set path to the directory and filename to the file
        path_ = ROOT_DIR + clean_url.substr(0, last_slash);
        filename_ = (last_slash + 1 < clean_url.length()) ? clean_url.substr(last_slash + 1) : "index.html";
    }

    // Ensure the path is not empty
    if (path_.empty())
    {
        path_ = ROOT_DIR;
    }

    // Step 4: Check if the path is a directory
    std::string fullpath = path_ + "/" + filename_;
    if (isDirectory(fullpath))
    {
        path_ = fullpath;         // Update path to the directory path
        filename_ = "index.html"; // Default to return index.html
    }

    // Step 5: Check if the path is valid and safe
    return isValidAndSafePath(path_);
}

bool HttpData::isDirectory(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool HttpData::isValidAndSafePath(const std::string& path)
{
    char resolvedPath[PATH_MAX];
    if (realpath(path.c_str(), resolvedPath) == nullptr)
    {
        return false; // Path resolution failed
    }
    std::string resolvedPathStr(resolvedPath);
    return resolvedPathStr.find(ROOT_DIR) == 0; // Check if path starts with ROOT_DIR
}

bool HttpData::parseHttpVersion(const std::string& version)
{
    if (version == "HTTP/1.0")
    {
        httpVersion_ = HttpVersion::HTTP_10;
    }
    else if (version == "HTTP/1.1")
    {
        httpVersion_ = HttpVersion::HTTP_11;
    }
    else
    {
        return false; // not a valid HTTP version
    }

    return true;
}

HttpData::HeaderState HttpData::parseHeader()
{
    while (true)
    {
        // Step 1: Check the end position of the current line
        size_t lineEnd = inBuffer_.find("\r\n", readIdx_);
        if (lineEnd == std::string::npos)
        {
            return HeaderState::AGAIN; // Incomplete data, need more data
        }

        // Step 2: Check if the line is empty, empty line indicates the end of header parsing
        if (lineEnd == readIdx_)
        {
            readIdx_ += 2; // Skip \r\n
            break;
        }

        // Step 3: Check the position of the colon to separate the header key and value
        size_t colonPos = inBuffer_.find(':', readIdx_);
        if (colonPos == std::string::npos || colonPos > lineEnd)
        {
            return HeaderState::ERROR; // Invalid header format
        }

        // Step 4: Extract header key
        std::string key = inBuffer_.substr(readIdx_, colonPos - readIdx_);
        key = trimTrailingSpaces(key); // Remove trailing spaces from key
        if (key.empty())
        {
            return HeaderState::ERROR; // Invalid header key
        }

        // Step 5: Extract header value
        size_t valueStart = colonPos + 1;
        while (valueStart < lineEnd && (inBuffer_[valueStart] == ' ' || inBuffer_[valueStart] == '\t'))
        {
            ++valueStart; // Skip leading spaces from value
        }

        std::string value = inBuffer_.substr(valueStart, lineEnd - valueStart);
        value = trimTrailingSpaces(value); // Remove trailing spaces from value
        if (value.empty())
        {
            return HeaderState::ERROR; // Invalid header value
        }

        // Step 6: Save key-value pair to header table
        headerFields_[key] = value;

        // Step 7: Update read position, process next line
        readIdx_ = lineEnd + 2; // Skip \r\n
    }
    return HeaderState::SUCCESS;
}

std::string HttpData::trimTrailingSpaces(const std::string& str)
{
    size_t end = str.find_last_not_of(" \t");
    if (end == std::string::npos)
    {
        return ""; // All spaces or empty string
    }
    return str.substr(0, end + 1);
}

HttpData::AnalyzeState HttpData::generateSendHTTP()
{
    // Get file type
    std::string filetype = getFileType(filename_);

    // don't support POST method
    if (httpMethod_ == HttpMethod::POST)
    {
        sendErrorHttp(fd_, 501, "Not Implemented");
        return AnalyzeState::ERROR;
    }

    // build response header
    std::string header = buildResponseHeader(filetype);

    // Handle GET or HEAD methods
    if (httpMethod_ == HttpMethod::GET || httpMethod_ == HttpMethod::HEAD)
    {
        // Special file handling: "hello"
        if (filename_ == "hello")
        {
            return handleHelloRequest();
        }

        // Special file handling: "index.html" (file listing)
        if (filename_ == "index.html")
        {
            return handleIndexRequest(header);
        }

        // Regular file handling
        return handleFileRequest(header, filetype);
    }

    // Unsupported HTTP method
    sendErrorHttp(fd_, 501, "Not Implemented");
    return AnalyzeState::ERROR;
}

void HttpData::sendErrorHttp(int fd, int err_num, std::string msg)
{
    msg = " " + msg;
    char send_buffer[4096];
    std::string header_buffer, body_buffer;
    body_buffer += "<html><title>Error</title>";
    body_buffer += "<body bgcolor=\"ffffff\">";
    body_buffer += std::to_string(err_num) + msg;
    body_buffer += "<hr><em> MerceMay's Web Server</em>\n</body></html>";

    header_buffer += "HTTP/1.1 " + std::to_string(err_num) + msg + "\r\n";
    header_buffer += "Content-Type: text/html\r\n";
    header_buffer += "Connection: Close\r\n";
    header_buffer += "Content-Length: " + std::to_string(body_buffer.size()) + "\r\n";
    header_buffer += "Server: MerceMay's Web Server\r\n";
    header_buffer += "\r\n";

    sprintf(send_buffer, "%s", header_buffer.c_str());
    writen(fd, send_buffer, strlen(send_buffer));
    sprintf(send_buffer, "%s", body_buffer.c_str());
    writen(fd, send_buffer, strlen(send_buffer));
}

std::string HttpData::getFileType(const std::string& filename)
{
    size_t dot_pos = filename_.find('.');
    if (dot_pos == std::string::npos)
    {
        return getMime("default");
    }
    return getMime(filename_.substr(dot_pos));
}

std::string HttpData::buildResponseHeader(const std::string& filetype)
{
    std::string header = (filetype == "video/mp4") ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";

    if (headerFields_.find("Connection") != headerFields_.end())
    {
        std::string& connection = headerFields_["Connection"];
        if (connection == "Keep-Alive" || connection == "keep-alive")
        {
            isKeepAlive_ = true;
            header += "Connection: Keep-Alive\r\n";
            header += "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        else
        {
            isKeepAlive_ = false;
            header += "Connection: Close\r\n";
        }
    }
    return header;
}

HttpData::AnalyzeState HttpData::handleHelloRequest()
{
    outBuffer_ = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nHello World";
    return AnalyzeState::SUCCESS;
}

HttpData::AnalyzeState HttpData::handleIndexRequest(std::string& header)
{
    std::string body;
    if (!curPathFileList(path_, body))
    {
        sendErrorHttp(fd_, 404, "Not Found");
        return AnalyzeState::ERROR;
    }

    header += "Content-Type: text/html\r\n";
    header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    header += "Server: MerceMay's Web Server\r\n\r\n";

    outBuffer_ = header + body;
    return AnalyzeState::SUCCESS;
}

HttpData::AnalyzeState HttpData::handleFileRequest(std::string& header, const std::string& filetype)
{
    std::string fullPath = path_ + "/" + filename_;
    struct stat sbuf;

    if (stat(fullPath.c_str(), &sbuf) < 0)
    {
        sendErrorHttp(fd_, 404, "Not Found");
        return AnalyzeState::ERROR;
    }

    header += "Content-Type: " + filetype + "\r\n";
    if (filetype == "video/mp4")
    {
        header += "Content-Range: bytes 0-" + std::to_string(sbuf.st_size - 1) + "/" + std::to_string(sbuf.st_size) + "\r\n";
    }
    header += "Content-Length: " + std::to_string(sbuf.st_size) + "\r\n";
    header += "Server: MerceMay's Web Server\r\n\r\n";

    if (httpMethod_ == HttpMethod::HEAD)
    {
        outBuffer_ = header;
        return AnalyzeState::SUCCESS;
    }

    return sendFileContent(fullPath, header, sbuf.st_size);
}

HttpData::AnalyzeState HttpData::sendFileContent(const std::string& fullPath, const std::string& header, size_t fileSize)
{
    int src_fd = open(fullPath.c_str(), O_RDONLY);
    if (src_fd < 0)
    {
        sendErrorHttp(fd_, 404, "Not Found");
        return AnalyzeState::ERROR;
    }

    // use mmap to map the file into memory, which uses RAII to manage the resource
    struct MmapCloser
    {
        void* addr = nullptr;
        size_t length = 0;

        ~MmapCloser()
        {
            if (addr != nullptr && addr != MAP_FAILED)
            {
                munmap(addr, length);
            }
        }
    };

    MmapCloser mmapResource;
    mmapResource.length = fileSize;
    mmapResource.addr = mmap(nullptr, mmapResource.length, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);

    if (mmapResource.addr == MAP_FAILED)
    {
        sendErrorHttp(fd_, 404, "Not Found");
        return AnalyzeState::ERROR;
    }

    char* src_addr = static_cast<char*>(mmapResource.addr);
    outBuffer_ = header + std::string(src_addr, fileSize);

    return AnalyzeState::SUCCESS;
}

bool HttpData::curPathFileList(std::string path, std::string& body)
{
    // Calculate the relative path
    std::string relativePath = path.substr(ROOT_DIR.length());
    if (!relativePath.empty() && relativePath.back() != '/')
    {
        // Ensure the relative path ends with '/'
        relativePath += "/";
    }

    body += "<html><body><h1>Directory Listing</h1><ul>";
    DIR* dir = opendir(path.c_str()); // Open the directory using the current path
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name == "." || name == "..")
            {
                continue; // Skip current directory and parent directory
            }

            // get the full path of the file
            std::string fullPath = relativePath + name;

            // If it's a directory, append '/' to the path
            std::string entryPath = path + "/" + name;
            struct stat entryStat;
            if (stat(entryPath.c_str(), &entryStat) == 0 && S_ISDIR(entryStat.st_mode))
            {
                fullPath += "/";
            }

            // Add to HTML link
            body += "<li><a href=\"" + fullPath + "\">" + name + "</a></li>";
        }
        closedir(dir);
    }
    else
    {
        return false; // Failed to open the directory
    }
    body += "</ul></body></html>";
    return true;
}

void HttpData::handleConnect()
{
    unlinkTimer(); // Start handling the connection, so the timer can be canceled
    unsigned int event = channel_->getEvents();
    if (!error_ && connectionState_ == ConnectionState::CONNECTED) // Connection has been established
    {
        int timeout = isKeepAlive_ ? DEFAULT_KEEP_ALIVE_TIME : DEFAULT_EXPIRED_TIME;
        if (event != 0) // Connection has been established and events have occurred
        {
            if ((event & EPOLLIN) && (event & EPOLLOUT)) // Both read and write events, prioritize write events
            {
                event = EPOLLOUT;
            }
            event |= EPOLLET; // Edge-triggered, ensure performance
        }
        else
        {
            event |= (EPOLLIN | EPOLLET); // Connection has been established and no events occurred, set to read event
        }
        updateEventAndTimeout(channel_, event, timeout); // update events and timeout
    }
    else if (!error_ && connectionState_ == ConnectionState::DISCONNECTING && (event & EPOLLOUT)) // indicate that the connection is closing, but there are data to be sent
    {
        updateEvent(channel_, EPOLLOUT | EPOLLET); // set channel_ only handle write event
    }
    else // indicate that the connection should be closed
    {
        // pass the shared pointer to the current object to the loop, so that the loop can call handleClose() when it is safe
        loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
    }
}

void HttpData::handleRead()
{
    // anonymous lambda function, which avoids to use goto 
    [&]()
    {
        bool zero = false;
        int readBytes = readn(fd_, inBuffer_);

        // Connection is closing, directly clear the buffer
        if (connectionState_ == ConnectionState::DISCONNECTING)
        {
            inBuffer_.clear();
            return;
        }

        // Read error
        if (readBytes < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) // non-blocking read mode, but no data available
            {
                return;
            }
            else if (errno == ECONNRESET) // client disconnected
            {
                connectionState_ = ConnectionState::DISCONNECTING;
                error_ = true;
                inBuffer_.clear();
                return;
            }
            std::cerr << "Error State: cannot read" << std::endl;
            error_ = true;
            inBuffer_.clear();
            sendErrorHttp(fd_, 400, "Bad Request");
            return;
        }
        // read 0 bytes
        else if (readBytes == 0)
        {
            // There is a request but no data, possibly the request was interrupted, 
            // or the network data has not arrived.
            // Most likely, the peer has already closed the connection.
            // Treat it uniformly as the peer having closed the connection.
            connectionState_ = ConnectionState::DISCONNECTING;
            return;
        }

        // parse the request line
        if (processState_ == ProcessState::PARSE_URL)
        {
            URLState urlState = parseRequestLine();
            if (urlState == URLState::AGAIN)
            {
                return;
            }
            else if (urlState == URLState::PATHUNINVALID)
            {
                error_ = true;
                LOG("log") << "Path unsafe: " << path_;
                inBuffer_.clear();
                sendErrorHttp(fd_, 403, "Forbidden");
                return;
            }
            else if (urlState == URLState::ERROR)
            {
                error_ = true;
                LOG("log") << "Error in request line" << request_line;
                inBuffer_.clear();
                sendErrorHttp(fd_, 400, "Bad Request: error in request line");
                return;
            }
            else
            {
                LOG("log") << "Request line: " << request_line;
                processState_ = ProcessState::PARSE_HEADERS;
            }
        }

        // parse the headers
        if (processState_ == ProcessState::PARSE_HEADERS)
        {
            HeaderState headerState = parseHeader();
            if (headerState == HeaderState::AGAIN)
            {
                return;
            }
            else if (headerState == HeaderState::ERROR)
            {
                error_ = true;
                LOG("log") << "Error in headers";
                sendErrorHttp(fd_, 400, "Bad Request: error in headers");
                inBuffer_.clear();
                return;
            }
            else
            {
                if (httpMethod_ == HttpMethod::POST)
                {
                    processState_ = ProcessState::RECV_BODY;
                }
                else
                {
                    processState_ = ProcessState::ANALYZE;
                }
            }
        }

        // POST request, read the body
        if (processState_ == ProcessState::RECV_BODY)
        {
            int content_length = -1;
            if (headerFields_.find("Content-length") != headerFields_.end())
            {
                const std::string& contentLengthStr = headerFields_["Content-length"];
                // Validate that Content-length is a pure number
                if (!contentLengthStr.empty() && std::all_of(contentLengthStr.begin(), contentLengthStr.end(), ::isdigit))
                {
                    try
                    {
                        content_length = stoi(contentLengthStr);
                    }
                    catch (const std::exception& e)
                    {
                        error_ = true;
                        sendErrorHttp(fd_, 400, "Bad Request: Invalid Content-length");
                        LOG("log") << "Invalid Content-length: " << contentLengthStr;
                        inBuffer_.clear();
                        return;
                    }
                }
                else
                {
                    error_ = true;
                    sendErrorHttp(fd_, 400, "Bad Request: Invalid Content-length");
                    LOG("log") << "Invalid Content-length: " << contentLengthStr;
                    inBuffer_.clear();
                    return;
                }
            }
            else
            {
                error_ = true;
                sendErrorHttp(fd_, 400, "Bad Request: Lack of argument (Content-length)");
                LOG("log") << "Lack of argument (Content-length)";
                inBuffer_.clear();
                return;
            }
            if (static_cast<int>(inBuffer_.size()) < content_length) // indicate that the body is not complete
                return;
            processState_ = ProcessState::ANALYZE;
        }

        if (processState_ == ProcessState::ANALYZE)
        {
            AnalyzeState analyzeSate = generateSendHTTP();
            if (analyzeSate == AnalyzeState::ERROR)
            {
                error_ = true;
                return;
            }
            else
            {
                inBuffer_.erase(0, readIdx_); // remove the data that has been processed
                processState_ = ProcessState::FINISH;
                return;
            }
        }
    }();

    if (error_)
    {
        return;
    }

    unsigned int events = channel_->getEvents();

    // there is data to be sent
    if (!outBuffer_.empty())
    {
        events |= EPOLLOUT | EPOLLET; // add write event
    }

    if (processState_ == ProcessState::FINISH) // request has been processed
    {
        reset(); // reset state after processing the request
        // client has closed, server also closes
        if (connectionState_ == ConnectionState::DISCONNECTING)
        {
            loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this()));
            return;
        }

        // there is still unprocessed data in the buffer, indicating a keep-alive connection, register read event again
        if (!inBuffer_.empty())
        {
            events |= EPOLLIN | EPOLLET; // add read event
        }
    }
    else if (connectionState_ != ConnectionState::DISCONNECTED) // connection is not disconnected and reaching here indicates previous requests are not processed
    {
        events |= EPOLLIN | EPOLLET; // add read event, indicating there are unfinished requests
    }
    // update events
    updateEvent(channel_, events);
}

void HttpData::handleWrite()
{
    if (!error_ && connectionState_ != ConnectionState::DISCONNECTED)
    {
        unsigned int events = channel_->getEvents();

        // write data to file descriptor
        int writeResult = writen(fd_, outBuffer_);
        if (writeResult < 0)
        {
            // write failed, handle error
            std::cerr << "Error State: error in write" << std::endl;
            error_ = true;
            loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this())); // close connection
            return;
        }

        if (outBuffer_.empty())
        {
            // write operation completed, buffer is empty
            if (connectionState_ == ConnectionState::DISCONNECTING)
            {
                loop_->runInLoop(std::bind(&HttpData::handleClose, shared_from_this())); // close connection
            }
            else
            {
                // write operation completed, buffer is empty
                events |= EPOLLIN | EPOLLET;
                events &= ~EPOLLOUT; // remove write event
            }
        }
        else
        {
            // write buffer still has data, continue to listen for write events
            events |= EPOLLOUT | EPOLLET;
        }

        // update events
        updateEvent(channel_, events);
    }
}

void HttpData::updateEventAndTimeout(std::shared_ptr<Channel> channel, unsigned int newEvent, int timeout)
{
    unsigned int& event = channel->getEvents();
    event = newEvent;                    // modify events
    loop_->modChannel(channel, timeout); // update event listening status
}

void HttpData::updateEvent(std::shared_ptr<Channel> channel, unsigned int newEvent)
{
    unsigned int& event = channel->getEvents();
    event = newEvent;           // modify events
    loop_->modChannel(channel); // update event listening status
}

HttpData::HttpData(std::shared_ptr<EventLoop> loop, int fd)
    : loop_(loop),
      fd_(fd),
      error_(false),
      connectionState_(ConnectionState::CONNECTED),
      httpMethod_(HttpMethod::GET),
      httpVersion_(HttpVersion::HTTP_11),
      readIdx_(0),
      processState_(ProcessState::PARSE_URL),
      parseState_(ParseState::START),
      isKeepAlive_(false)
{
    channel_ = std::make_shared<Channel>(loop, fd);
    channel_->setConnectHandler(std::bind(&HttpData::handleConnect, this));
    channel_->setReadHandler(std::bind(&HttpData::handleRead, this));
    channel_->setWriteHandler(std::bind(&HttpData::handleWrite, this));
}

HttpData::~HttpData()
{
    unlinkTimer();
    if (channel_)
    {
        channel_->setHolder(nullptr);
        channel_->setFd(-1);
        channel_.reset();
    }
    close(fd_);
}

void HttpData::reset()
{
    readIdx_ = 0;
    processState_ = ProcessState::PARSE_URL;
    parseState_ = ParseState::START;

    filename_.clear();
    path_.clear();
    headerFields_.clear();

    unlinkTimer();
}

void HttpData::linkTimer(std::shared_ptr<TimerNode> mtimer)
{
    timer_ = mtimer;
}

void HttpData::unlinkTimer()
{
    if (auto t = timer_.lock())
    {
        t->clearHttpData();
        timer_.reset();
    }
}

std::shared_ptr<EventLoop> HttpData::getLoop()
{
    return loop_;
}

std::shared_ptr<Channel> HttpData::getChannel()
{
    return channel_;
}

void HttpData::handleClose()
{
    connectionState_ = ConnectionState::DISCONNECTED;
    unlinkTimer();
    std::shared_ptr<HttpData> guard(shared_from_this()); // move shared_ptr of this object to the guard
    loop_->removeChannel(channel_);                      // remove channel from EventLoop
}

void HttpData::newEvent()
{
    channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    loop_->addChannel(channel_, DEFAULT_EXPIRED_TIME);
}