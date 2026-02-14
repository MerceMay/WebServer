#pragma once

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include <map>
#include <string>

class Server
{
public:
    using ConnectionCallback = TcpConnection::ConnectionCallback;
    using MessageCallback = TcpConnection::MessageCallback;

    Server(EventLoop* loop, int threadNum, int port);
    ~Server();

    void start();
    
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const std::shared_ptr<TcpConnection>& conn);
    void removeConnectionInLoop(const std::shared_ptr<TcpConnection>& conn);

    using ConnectionMap = std::map<std::string, std::shared_ptr<TcpConnection>>;

    EventLoop* loop_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;
    std::unique_ptr<Acceptor> acceptor_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};
