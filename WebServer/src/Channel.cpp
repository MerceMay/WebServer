#include "Channel.h"

Channel::Channel(std::shared_ptr<EventLoop> loop)
    : loop_(loop),
      fd_(-1),
      events_(0)
{
}

Channel::Channel(std::shared_ptr<EventLoop> loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0)
{
}

void Channel::handleEvent(unsigned int activeEvents)
{
    if (activeEvents & EPOLLERR) // if there exists error event
    {
        if (errorHandler)
        {
            errorHandler();
        }
        return;
    }
    if ((activeEvents & EPOLLHUP) && !(activeEvents & EPOLLIN)) // if this channel is holding up and there is no read event
    {
        return; // do nothing
    }
    if (activeEvents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) // read event
    {
        if (readHandler)
        {
            readHandler();
        }
    }
    if (activeEvents & EPOLLOUT) // write event
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

