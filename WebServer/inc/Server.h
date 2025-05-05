#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"
#include "Util.h"
#include <arpa/inet.h>

class Server
{
private:
    std::shared_ptr<EventLoop> mainEventLoop_;         // main event loop
    int threadNum_;                                    // thread number
    std::unique_ptr<EventLoopThreadPool> threadPool_;  // thread pool
    bool started_;                                     // server started flag
    std::shared_ptr<Channel> acceptConnectionChannel_; // accept connection channel
    int port_;                                         // server port
    int listenFd_;                                     // listen file descriptor
    static const int MAX_LISTEN_FD = 65535;            // maximum listen file descriptor
public:
    Server(std::shared_ptr<EventLoop> loop, int threadNum, int port);
    ~Server() = default;
    Server() = delete;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void start(); // start the server

    void handleAccept(); // handle accept event

    void handleThisAccept(); // add listen channel to main event loop
};