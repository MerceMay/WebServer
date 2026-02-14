#pragma once

#include "EventLoop.h"
#include "Channel.h"
#include <functional>

class Acceptor
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>; // InetAddress not defined yet, use struct sockaddr_in?

    Acceptor(EventLoop* loop, int port);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    void listen();
    bool listening() const { return listening_; }

private:
    void handleRead();

    EventLoop* loop_;
    int acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};

// Simple InetAddress struct for now
struct InetAddress
{
    // Placeholder
};
