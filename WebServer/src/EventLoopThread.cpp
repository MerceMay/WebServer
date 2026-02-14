#include "EventLoopThread.h"
#include "EventLoop.h"
#include <assert.h>

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exiting_(false)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.joinable()); // Should not start twice
    thread_ = std::thread(std::bind(&EventLoopThread::threadFunc, this));

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    // Loop ends here
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
