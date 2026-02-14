#pragma once
#include <functional>
#include <chrono>
#include <set>
#include <vector>
#include <memory>
#include "Channel.h"

class EventLoop;

class TimerNode {
public:
    using TimerCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    using Timestamp = Clock::time_point;

    TimerNode(TimerCallback cb, Timestamp when, double interval)
        : callback_(cb), expiration_(when), interval_(interval), repeat_(interval > 0.0) {}

    void run() const { if (callback_) callback_(); }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    void restart(Timestamp now) { 
        expiration_ = now + std::chrono::milliseconds(static_cast<int64_t>(interval_ * 1000)); 
    }

private:
    TimerCallback callback_;
    Timestamp expiration_;
    double interval_;
    bool repeat_;
};

class TimerManager {
public:
    using TimerCallback = std::function<void()>;
    using Clock = std::chrono::steady_clock;
    using Timestamp = Clock::time_point;

    explicit TimerManager(EventLoop* loop);
    ~TimerManager();

    void addTimer(TimerCallback cb, Timestamp when, double interval);

private:
    using Entry = std::pair<Timestamp, TimerNode*>;
    using TimerList = std::set<Entry>;

    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(TimerNode* timer);

    EventLoop* loop_;
    int timerfd_;
    std::unique_ptr<Channel> timerfdChannel_;
    TimerList timers_;
};
