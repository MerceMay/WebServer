#include "Epoll.h"
#include "Channel.h"
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <iostream>

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

Epoll::Epoll()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollFd_ < 0)
    {
        perror("epoll_create1");
    }
}

Epoll::~Epoll()
{
    close(epollFd_);
}

void Epoll::updateChannel(Channel* channel)
{
    const int index = channel->index();
    int fd = channel->fd();
    
    if (index == kNew || index == kDeleted)
    {
        // Add new channel
        struct epoll_event ev;
        bzero(&ev, sizeof ev);
        ev.events = channel->events();
        ev.data.ptr = channel;
        
        if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            perror("epoll_ctl add");
        }
        channel->setIndex(kAdded);
    }
    else
    {
        // Update existing channel
        struct epoll_event ev;
        bzero(&ev, sizeof ev);
        ev.events = channel->events();
        ev.data.ptr = channel;
        
        if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0)
        {
            perror("epoll_ctl mod");
        }
    }
}

void Epoll::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    int index = channel->index();
    
    if (index == kAdded)
    {
        if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) < 0)
        {
            perror("epoll_ctl del");
        }
    }
    channel->setIndex(kNew);
}

std::vector<Channel*> Epoll::poll(int timeoutMs)
{
    int numEvents = epoll_wait(epollFd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    
    std::vector<Channel*> activeChannels;
    
    if (numEvents > 0)
    {
        for (int i = 0; i < numEvents; ++i)
        {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            activeChannels.push_back(channel);
        }
        
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        // Timeout
    }
    else
    {
        if (savedErrno != EINTR)
        {
            perror("epoll_wait");
        }
    }
    
    return activeChannels;
}
