#include "EventLoopThread.h"

void EventLoopThread::threadFunc()
{
    {
        std::unique_lock<std::mutex> lock(mutex_); // Lock to protect shared resources
        loop_ = std::make_shared<EventLoop>();     // Create an EventLoop object
        loop_->initEventChannel();                 // Initialize the event channel
        cond_.notify_all();                        // Notify the main thread that the event loop is created
        }
        loop_->loop(); // Start the event loop
}

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      exit_(false),
      mutex_(),
      thread_(),
      cond_()
{
}

EventLoopThread::~EventLoopThread()
{
    exit_ = true;
    cond_.notify_all();
    if (thread_.joinable() && loop_)
    {
        loop_->quit();  // quit the event loop
        thread_.join(); // wait for the thread to finish
    }
}

std::weak_ptr<EventLoop> EventLoopThread::startLoop()
{
    assert(!loop_);                                            // Ensure that the event loop has not been created yet
    assert(!exit_);                                            // Ensure that we are not in exit state
    thread_ = std::thread(&EventLoopThread::threadFunc, this); // Create thread
    {
        std::unique_lock<std::mutex> lock(mutex_); // Lock to protect
        while (loop_ == nullptr)
        {
            cond_.wait(lock); // Wait for the event loop to be created
        }
    }
    return loop_->weak_from_this(); // Return the weak pointer to the event loop
}
