#include "Server.h"
#include "Util.h"
#include <iostream>
#include <functional>

Server::Server(EventLoop* loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      threadPool_(std::make_unique<EventLoopThreadPool>(loop, threadNum)),
      acceptor_(std::make_unique<Acceptor>(loop, port)),
      started_(false),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&Server::newConnection, this, std::placeholders::_1, std::placeholders::_2));
    handle_for_sigpipe();
}

Server::~Server()
{
    for (auto& item : connections_)
    {
        std::shared_ptr<TcpConnection> conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void Server::start()
{
    if (!started_)
    {
        started_ = true;
        threadPool_->start();
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void Server::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop* ioLoop = threadPool_->getNextLoop();
    
    std::string connName = "Connection-" + std::to_string(nextConnId_++);
    
    // LOG_INFO << "Server::newConnection [" << connName << "] - new connection";
    setSocketNodelay(sockfd);
    
    std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd);
    connections_[connName] = conn;
    
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(std::bind(&Server::removeConnection, this, std::placeholders::_1));
    
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void Server::removeConnection(const std::shared_ptr<TcpConnection>& conn)
{
    loop_->runInLoop(std::bind(&Server::removeConnectionInLoop, this, conn));
}

void Server::removeConnectionInLoop(const std::shared_ptr<TcpConnection>& conn)
{
    loop_->assertInLoopThread();
    size_t n = connections_.erase(conn->name());
    (void)n;
    
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
