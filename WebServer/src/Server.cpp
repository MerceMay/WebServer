#include "Server.h"

Server::Server(std::shared_ptr<EventLoop> loop, int threadNum, int port)
    : mainEventLoop_(loop),
      threadNum_(threadNum),
      port_(port),
      started_(false)
{
    threadPool_ = std::make_unique<EventLoopThreadPool>(loop, threadNum);
    if (port < 0 || port > 65535)
    {
        port = 80;
    }
    listenFd_ = socket_bind_listen(port);
    if (listenFd_ < 0)
    {
        LOG("log") << "socket_bind_listen error";
        exit(EXIT_FAILURE);
    }
    acceptConnectionChannel_ = std::make_shared<Channel>(loop, listenFd_);
    handle_for_sigpipe(); // ignore SIGPIPE signal
    if (setSocketNonBlocking(listenFd_) < 0)
    {
        LOG("log") << "setSocketNonBlocking error";
        exit(EXIT_FAILURE);
    }
}

void Server::start()
{
    threadPool_->start();
    acceptConnectionChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptConnectionChannel_->setReadHandler(std::bind(&Server::handleAccept, this));
    acceptConnectionChannel_->setConnectHandler(std::bind(&Server::handleThisAccept, this));
    mainEventLoop_->addChannel(acceptConnectionChannel_, 0); // add accept connection channel to main event loop, no timeout set
    started_ = true;
}

void Server::handleAccept()
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = 0;
    while (true)
    {
        connfd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len);
        if (connfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // if no more connections, exit the loop and end accept event
            }
            else if (errno == EINTR)
            {
                continue; // interrupted by signal, continue accepting connections
            }
            else
            {
                LOG("log") << "Server accept a new connection error";
                break;
            }
        }
        std::weak_ptr<EventLoop> nextEventLoop = threadPool_->getNextLoop();
        std::shared_ptr<EventLoop> ioLoop = nextEventLoop.lock();
        if (!ioLoop)
        {
            LOG("log") << "Get next loop error";
            close(connfd);
            break;
        }
        LOG("log") << "New connection from "
                   << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port)
                   << " fd: " << connfd;
        if (connfd >= MAX_LISTEN_FD)
        {
            LOG("log") << "connfd >= MAX_LISTEN_FD";
            close(connfd);
            continue;
        }
        if (setSocketNonBlocking(connfd) < 0)
        {
            LOG("log") << "Set socket non-blocking error";
            return;
        }
        setSocketNodelay(connfd);
        std::shared_ptr<HttpData> newHttpData = std::make_shared<HttpData>(ioLoop, connfd);
        newHttpData->getChannel()->setHolder(newHttpData);
        ioLoop->queueInLoop(std::bind(&HttpData::newEvent, newHttpData));
    }
    acceptConnectionChannel_->setRevents(EPOLLIN | EPOLLET);
}

void Server::handleThisAccept()
{
    mainEventLoop_->modChannel(acceptConnectionChannel_); // modify accept connection channel
}
