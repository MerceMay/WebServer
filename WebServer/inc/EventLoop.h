#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

class Epoll;
class Channel;
class TimerManager;

class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void loop();
    void quit();

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    // Timers
    void runAt(std::chrono::steady_clock::time_point time, std::function<void()> cb);
    void runAfter(double delay, std::function<void()> cb);
    void runEvery(double interval, std::function<void()> cb);

    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }
    void assertInLoopThread();

private:
    void handleRead(); // Wakeup handler
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> eventHandling_;
    std::atomic<bool> callingPendingFunctors_;
    
    const std::thread::id threadId_;
    std::unique_ptr<Epoll> poller_;
    
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    std::unique_ptr<TimerManager> timerQueue_;
    
    ChannelList activeChannels_;
    
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
};
