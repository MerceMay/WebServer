#pragma once
#include "HttpData.h"
#include <chrono>
#include <memory>
#include <queue>

class HttpData;

class TimerNode
{
private:
    bool isValid_;
    std::chrono::system_clock::time_point expire_time_point;
    std::weak_ptr<HttpData> httpData_;

public:
    TimerNode(std::shared_ptr<HttpData> httpData, int milli_timeout);
    ~TimerNode();

    TimerNode(const TimerNode& tn);
    TimerNode& operator=(const TimerNode& tn);

    void update(int milli_timeout);
    bool isValid();
    void clearHttpData(); // clear the weak pointer of HttpData

    void expired();
    bool isDeleted();
    std::chrono::system_clock::time_point getExpireTime();
};

class TimerManager
{
public:
    TimerManager() = default;
    ~TimerManager() = default;
    TimerManager(const TimerManager&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;
    TimerManager(TimerManager&&) = delete;
    TimerManager& operator=(TimerManager&&) = delete;
    void addTimerNode(std::shared_ptr<HttpData> httpData, int milli_timeout);

    void handleExpiredEvent();

private:
    struct TimerCmp
    {
        bool operator()(const std::shared_ptr<TimerNode>& lhs, const std::shared_ptr<TimerNode>& rhs) const
        {
            return lhs->getExpireTime() > rhs->getExpireTime();
        }
    };

    std::priority_queue<std::shared_ptr<TimerNode>, std::vector<std::shared_ptr<TimerNode>>, TimerCmp> timerQueue;
};
