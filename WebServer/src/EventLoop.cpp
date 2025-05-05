#include "EventLoop.h"

void EventLoop::doPendingFunctions()
{
    std::vector<std::function<void()>> functions;
    {
        std::lock_guard<std::mutex> lock(mutex_); // lock the mutex to protect pendingFunctions_
        functions.swap(pendingFunctions_);        // swap the pending functions queue
    }
    isCallingPendingFunctions_ = true; // set the flag for calling pending functions
    for (const auto& func : functions) // iterate over pending functions
    {
        func(); // execute function
    }
    isCallingPendingFunctions_ = false; // reset the flag for calling pending functions
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(eventFd_, &one, sizeof(one)); // wake up EventLoop.
    if (n != sizeof(one))
    {
        LOG("log") << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(eventFd_, &one, sizeof(one)); // read event fd
    if (n != sizeof(one))
    {
        LOG("log") << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    eventChannel_->setRevents(EPOLLIN | EPOLLET); // set rightnow events to read mode
}

void EventLoop::handleConnect()
{
    modChannel(eventChannel_); // set last event to event
}

void EventLoop::initEventChannel()
{
    eventChannel_ = std::make_shared<Channel>(shared_from_this(), eventFd_); // create event channel

    eventChannel_->setEvents(EPOLLIN | EPOLLET);                                  // set event type to read and edge-triggered
    eventChannel_->setReadHandler(std::bind(&EventLoop::handleRead, this));       // bind read event handler
    eventChannel_->setConnectHandler(std::bind(&EventLoop::handleConnect, this)); // bind connect event handler
    epoll_->add_epoll(eventChannel_, 0);
}

EventLoop::EventLoop()
    : isLooping_(false),
      shouldStop_(false),
      epoll_(std::make_shared<Epoll>()),
      isHandlingEvent_(false),
      isCallingPendingFunctions_(false),
      threadId_(std::this_thread::get_id()),
      mutex_()
{
    pendingFunctions_ = {};
    eventFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // create eventfd for wakeup
    if (eventFd_ < 0)                                  // check if event file descriptor was created successfully
    {
        LOG("log") << "eventfd error";
        exit(EXIT_FAILURE);
    }
}

EventLoop::~EventLoop()
{
    shouldStop_ = true; // set the stop flag
    if (eventFd_ > 0)
    {
        close(eventFd_); // close event file descriptor
    }
}

bool EventLoop::isEventLoopInOwnThread()
{
    return std::this_thread::get_id() == threadId_; // check if current thread ID is the same as EventLoop thread ID
}

void EventLoop::loop()
{
    assert(!isLooping_);              // check if already looping
    assert(isEventLoopInOwnThread()); // check if in the same thread
    isLooping_ = true;                // set looping flag
    shouldStop_ = false;              // set stop flag
    while (!shouldStop_)              // loop until stop
    {
        std::vector<std::shared_ptr<Channel>> activeChannels = epoll_->runEpoll(); // get active channels from epoll
        isHandlingEvent_ = true;                                                   // set handling event flag
        for (const auto& channel : activeChannels)                                 // iterate over active channels
        {
            channel->handleEvent(); // handle channel_'s event
        }
        isHandlingEvent_ = false; // reset handling event flag
        doPendingFunctions();     // execute pending functions
    }
    isLooping_ = false; // reset looping flag
}

void EventLoop::quit()
{
    shouldStop_ = true;            // set stop flag
    if (!isEventLoopInOwnThread()) // if not in current thread
    {
        wakeup(); // wake up EventLoop
    }
}

void EventLoop::runInLoop(std::function<void()> func)
{
    if (isEventLoopInOwnThread())
    {
        func(); // if in current thread, execute function
    }
    else
    {
        queueInLoop(func); // if not in current thread, queue function
    }
}

void EventLoop::queueInLoop(std::function<void()> func)
{
    {
        std::lock_guard<std::mutex> lock(mutex_); // lock protection
        pendingFunctions_.emplace_back(func);     // add function to pending queue
    }
    if (!isEventLoopInOwnThread() || isCallingPendingFunctions_) // if not in current thread or calling pending functions
    {
        wakeup(); // wake up EventLoop
    }
}

void EventLoop::shutdown(std::shared_ptr<Channel> request)
{
    if (isEventLoopInOwnThread())
    {
        shutDownWR(request->getFd()); // close write end
    }
    else
    {
        runInLoop(std::bind(&EventLoop::shutdown, this, request)); // run shutdown function in current thread
    }
}

void EventLoop::removeChannel(std::shared_ptr<Channel> request)
{
    epoll_->del_epoll(request);
}

void EventLoop::addChannel(std::shared_ptr<Channel> request, int timeout)
{
    epoll_->add_epoll(request, timeout);
}

void EventLoop::modChannel(std::shared_ptr<Channel> request, int timeout)
{
    epoll_->mod_epoll(request, timeout);
}

void EventLoop::modChannel(std::shared_ptr<Channel> request)
{
    epoll_->mod_epoll(request);
}