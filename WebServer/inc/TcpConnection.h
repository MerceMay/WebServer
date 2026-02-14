#pragma once

#include "EventLoop.h"
#include "Buffer.h"
#include <memory>
#include <string>
#include <atomic>
#include <any>

class Channel;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using CloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*)>;

    TcpConnection(EventLoop* loop, const std::string& name, int sockfd);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    int fd() const { return fd_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    void send(const std::string& message);
    void send(Buffer* message);
    void shutdown();
    void forceClose();

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    void setContext(const std::any& context) { context_ = context; }
    const std::any& getContext() const { return context_; }
    std::any* getMutableContext() { return &context_; }

    // Internal use
    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    
    void sendInLoop(const std::string& message);
    void sendInLoop(const char* data, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();

    EventLoop* loop_;
    const std::string name_;
    int fd_;
    std::atomic<StateE> state_;
    std::unique_ptr<Channel> channel_;
    
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    
    std::any context_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;
};
