#pragma once
#include "EventLoop.h"
#include <condition_variable>
#include <mutex>

class EventLoopThread
{
private:
    std::shared_ptr<EventLoop> loop_; // Each EventLoopThread has an EventLoop object
    std::thread thread_;              // Thread object
    std::mutex mutex_;                // Mutex to protect loop_
    std::condition_variable cond_;    // Condition variable to notify the event loop thread
    bool exit_;                       // Exit flag

    void threadFunc(); // Event loop thread function

public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;
    EventLoopThread(EventLoopThread&&) = delete;
    EventLoopThread& operator=(EventLoopThread&&) = delete;

    std::weak_ptr<EventLoop> startLoop(); // Start the event loop thread
};