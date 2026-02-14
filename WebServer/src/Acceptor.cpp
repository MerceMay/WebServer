#include "Acceptor.h"
#include "Util.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

Acceptor::Acceptor(EventLoop* loop, int port)
    : loop_(loop),
      acceptSocket_(socket_bind_listen(port)),
      acceptChannel_(loop, acceptSocket_),
      listening_(false)
{
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    close(acceptSocket_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int connfd = accept4(acceptSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            InetAddress addr; // TODO: Fill addr
            newConnectionCallback_(connfd, addr);
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        perror("Acceptor::handleRead");
        if (errno == EMFILE)
        {
            // TODO: handle EMFILE gracefully
        }
    }
}
