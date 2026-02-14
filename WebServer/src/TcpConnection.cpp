#include "TcpConnection.h"
#include "Channel.h"
#include <unistd.h>
#include <iostream>

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd)
    : loop_(loop),
      name_(name),
      fd_(sockfd),
      state_(kConnecting),
      channel_(std::make_unique<Channel>(loop, sockfd))
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
    // Close fd in destructor (RAII)
    if (fd_ >= 0)
    {
        close(fd_);
    }
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    state_ = kConnected;
    channel_->tie(shared_from_this());
    channel_->enableReading();
    
    if (connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        state_ = kDisconnected;
        channel_->disableAll();
        
        if (connectionCallback_)
        {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove(); // Need to ensure Channel has remove()
}

void TcpConnection::handleRead()
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(fd_, &savedErrno);
    
    if (n > 0)
    {
        if (messageCallback_)
        {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        ssize_t n = write(fd_, outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                // TODO: writeCompleteCallback
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            perror("TcpConnection::handleWrite");
        }
    }
}

void TcpConnection::handleClose()
{
    state_ = kDisconnected;
    channel_->disableAll();
    
    std::shared_ptr<TcpConnection> guardThis(shared_from_this());
    if (connectionCallback_)
    {
        connectionCallback_(guardThis);
    }
    
    if (closeCallback_)
    {
        closeCallback_(guardThis);
    }
}

void TcpConnection::handleError()
{
    int err = 0;
    socklen_t len = sizeof err;
    getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    // LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

void TcpConnection::send(const std::string& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message.data(), message.size());
        }
        else
        {
            loop_->runInLoop(std::bind((void (TcpConnection::*)(const std::string&))&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::send(Buffer* message)
{
     if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message->peek(), message->readableBytes());
            message->retrieveAll();
        }
        else
        {
            loop_->runInLoop([this, str = message->retrieveAllAsString()]() {
                sendInLoop(str.data(), str.size());
            });
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const char* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) return;

    // if no thing in output queue, try to write directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = write(fd_, data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0) // complete
            {
                // TODO: writeCompleteCallback
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                perror("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0)
    {
        outputBuffer_.append(data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        state_ = kDisconnecting;
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())
    {
        ::shutdown(fd_, SHUT_WR);
    }
}

void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        state_ = kDisconnecting;
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        handleClose();
    }
}
