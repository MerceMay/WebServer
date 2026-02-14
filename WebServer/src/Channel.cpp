#include "Channel.h"
#include "EventLoop.h"
#include <unistd.h>
#include <cstdlib>
#include <iostream>

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent()
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard();
        }
    }
    else
    {
        handleEventWithGuard();
    }
}

void Channel::handleEventWithGuard()
{
    // LOG_INFO << "Channel handleEvent revents" << revents_;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        if (readCallback_) readCallback_();
    }

    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}
