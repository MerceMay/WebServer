#include "Epoll.h"
#include "Timer.h"

Epoll::Epoll()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)) // create epoll instance
{
    timerManager_ = std::make_unique<TimerManager>(); // create timer manager
    assert(epollFd_ > 0);                             // check if epoll instance is created successfully
    events_.resize(MAXFDS);                           // resize events_ vector to maximum number of file descriptors
    channels_.resize(MAXFDS);
    httpDatas_.resize(MAXFDS);
}

bool Epoll::add_epoll(std::shared_ptr<Channel> request, int timeout)
{
    int fd = request->getFd();
    if (fd < 0)
    {
        return false;
    }

    if (timeout > 0)
    {
        addTimerNode(request, timeout);                            // add timer node
        std::shared_ptr<HttpData> httpData = request->getHolder(); // add corresponding HttpData to httpDatas_
        httpDatas_[fd] = httpData;                                 // store HttpData in httpDatas_
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents(); // get event type

    channels_[fd] = request; // store Channel in channels_

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        LOG("log") << "epoll_ctl add error";
        channels_[fd].reset(); // remove from channels_ on failure
        return false;
    }
    return true;
}

bool Epoll::mod_epoll(std::shared_ptr<Channel> request, int timeout)
{
    if (timeout > 0)
    {
        addTimerNode(request, timeout); // add timer node
    }
    int fd = request->getFd();
    if (fd < 0)
    {
        return false;
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents(); // get event type
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        LOG("log") << "epoll_ctl mod error";
        channels_[fd].reset(); // remove from channels_ on failure
        return false;
    }
    return true;
}

bool Epoll::mod_epoll(std::shared_ptr<Channel> request)
{
    int fd = request->getFd();
    if (fd < 0)
    {
        return false;
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents(); // get event type
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        LOG("log") << "epoll_ctl mod error";
        channels_[fd].reset(); // remove from channels_ on failure
        return false;
    }
    return true;
}

bool Epoll::del_epoll(std::shared_ptr<Channel> request)
{
    int fd = request->getFd();
    if (fd < 0)
    {
        return false;
    }
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) < 0)
    {
        LOG("log") << "epoll_ctl del error";
        return false;
    }
    channels_[fd].reset();  // remove fd and Channel association
    httpDatas_[fd].reset(); // remove fd and HttpData association
    return true;
}

bool Epoll::addTimerNode(std::shared_ptr<Channel> request, int timeout)
{
    std::shared_ptr<HttpData> holder = request->getHolder();
    if (holder)
    {
        timerManager_->addTimerNode(holder, timeout); // add timer
        return true;
    }
    else
    {
        LOG("log") << "timer add error";
        return false;
    }
}

std::vector<ChannelEvent> Epoll::runEpoll()
{
    while (true)
    {
        int event_count = epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), EPOLL_TIMEOUT);
        if (event_count < 0)
        {
            LOG("log") << "epoll_wait error";
            exit(EXIT_FAILURE);
        }
        else if (event_count == 0)
        {
            handleExpired(); // handle expired timer events
            continue;
        }
        else
        {
            std::vector<ChannelEvent> activeChannels = getEventsRequest(event_count); // get active Channels
            if (activeChannels.size() > 0)
            {
                return activeChannels;
            }
        }
    }
}

std::vector<ChannelEvent> Epoll::getEventsRequest(int events_sum)
{

    std::vector<ChannelEvent> activeChannels;
    for (int i = 0; i < events_sum; ++i)
    {
        int fd = events_[i].data.fd;
        std::shared_ptr<Channel> cur_channel = channels_[fd];
        if (cur_channel)
        {
            cur_channel->setFd(fd);
            unsigned int activeEvents = events_[i].events;
            activeChannels.emplace_back(cur_channel, activeEvents);
        }
    }
    return activeChannels;
}

void Epoll::handleExpired()
{
    timerManager_->handleExpiredEvent();
}

int Epoll::getEpollFd() const
{
    return epollFd_;
}
