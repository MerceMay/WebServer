#pragma once

#include "HttpData.h"
#include "Timer.h"
#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;
class HttpData;

class Channel : public std::enable_shared_from_this<Channel>
{
private:
    std::shared_ptr<EventLoop> loop_; // event loop
    int fd_;
    unsigned int events_;         // event types to register for next epoll operation (EPOLLIN, EPOLLOUT, EPOLLET)
    unsigned int rightnowEvents_; // event types that are currently active (set by epoll_wait results)
    unsigned int lastEvents_;     // last registered event types (used for epoll update optimization)

    std::weak_ptr<HttpData> holder_; // get the HttpData object which this channel is associated

private:
    std::function<void()> readHandler;
    std::function<void()> writeHandler;
    std::function<void()> errorHandler;
    std::function<void()> connectHandler;

public:
    Channel(std::shared_ptr<EventLoop> loop);

    Channel(std::shared_ptr<EventLoop> loop, int fd);

    Channel() = delete;
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    ~Channel() {}

    int getFd() const { return fd_; }
    void setFd(int fd) { fd_ = fd; }

    void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
    std::shared_ptr<HttpData> getHolder() { return holder_.lock(); }

    void setReadHandler(std::function<void()> handler) { readHandler = handler; }
    void setWriteHandler(std::function<void()> handler) { writeHandler = handler; }
    void setErrorHandler(std::function<void()> handler) { errorHandler = handler; }
    void setConnectHandler(std::function<void()> handler) { connectHandler = handler; }

    void handleEvent();

    void setEvents(unsigned int events) { events_ = events; }
    unsigned int& getEvents() { return events_; }
    void setRevents(unsigned int revents) { rightnowEvents_ = revents; }

    // Synchronizes lastEvents_ with events_ and returns true if they were equal before sync
    // Used for epoll update optimization: if events haven't changed, skip epoll_ctl call
    bool isEqualAndSetLastEventsToEvents();
};