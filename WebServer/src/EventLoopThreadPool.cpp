#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(std::shared_ptr<EventLoop> mainLoop, int numThreads)
    : mainEventLoop_(mainLoop),
      started_(false),
      numThreads_(numThreads),
      nextIndex_(0)
{
    if (numThreads_ <= 0)
    {
        LOG("log") << "numThreads_ <= 0";
        exit(EXIT_FAILURE);
    }
}

void EventLoopThreadPool::start()
{
    assert(mainEventLoop_->isEventLoopInOwnThread()); // make sure the main event loop is in the main thread
    assert(!started_);
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        threads_.push_back(std::make_unique<EventLoopThread>());
        loops_.push_back(threads_[i]->startLoop());
    }
}

std::weak_ptr<EventLoop> EventLoopThreadPool::getNextLoop()
{
    assert(mainEventLoop_->isEventLoopInOwnThread()); // make sure the main event loop is in the main thread
    assert(started_);
    std::weak_ptr<EventLoop> loop;
    if (!loops_.empty())
    {
        loop = loops_[nextIndex_];
        nextIndex_ = (nextIndex_ + 1) % loops_.size();
    }
    return loop;
}