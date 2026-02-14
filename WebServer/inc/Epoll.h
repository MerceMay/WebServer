#pragma once

#include <sys/epoll.h>
#include <vector>
#include <memory>

class Channel;

class Epoll
{
public:
    Epoll();
    ~Epoll();

    // Non-copyable
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    std::vector<Channel*> poll(int timeoutMs);

private:
    static const int kInitEventListSize = 16;

    int epollFd_;
    std::vector<struct epoll_event> events_;
};
