#include "Channel.h"

Channel::Channel(std::shared_ptr<EventLoop> loop)
    : loop_(loop),
      fd_(-1),
      events_(0),
      rightnowEvents_(0),
      lastEvents_(0)
{
}

Channel::Channel(std::shared_ptr<EventLoop> loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      rightnowEvents_(0),
      lastEvents_(0)
{
}

void Channel::handleEvent()
{
    if (rightnowEvents_ & EPOLLERR) // if there exists error event
    {
        if (errorHandler)
        {
            errorHandler();
        }
        return;
    }
    if ((rightnowEvents_ & EPOLLHUP) && !(rightnowEvents_ & EPOLLIN)) // if this channel is holding up and there is no read event
    {
        return; // do nothing
    }
    if (rightnowEvents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) // read event
    {
        if (readHandler)
        {
            readHandler();
        }
    }
    if (rightnowEvents_ & EPOLLOUT) // write event
    {
        if (writeHandler)
        {
            writeHandler();
        }
    }
    if (connectHandler) // connect event
    {
        connectHandler();
    }
}

bool Channel::isEqualAndSetLastEventsToEvents()
{
    bool isEqual = (events_ == lastEvents_);
    lastEvents_ = events_; // Always sync lastEvents_ with current events_
    return isEqual;
}