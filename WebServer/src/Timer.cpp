#include "Timer.h"
#include "EventLoop.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

using namespace std;
using namespace std::chrono;

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        // LOG << "Failed in timerfd_create";
    }
    return timerfd;
}

void readTimerfd(int timerfd, TimerNode::Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        // LOG << "Timer::readTimerfd() reads " << n << " bytes instead of 8";
    }
}

void resetTimerfd(int timerfd, TimerNode::Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    std::memset(&newValue, 0, sizeof newValue);
    std::memset(&oldValue, 0, sizeof oldValue);

    int64_t microseconds = duration_cast<std::chrono::microseconds>(
        expiration - steady_clock::now()).count();
    
    if (microseconds < 100) microseconds = 100;

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / 1000000);
    ts.tv_nsec = static_cast<long>((microseconds % 1000000) * 1000);
    
    newValue.it_value = ts;
    ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
}

TimerManager::TimerManager(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(new Channel(loop, timerfd_))
{
    timerfdChannel_->setReadCallback(std::bind(&TimerManager::handleRead, this));
    timerfdChannel_->enableReading();
}

TimerManager::~TimerManager()
{
    timerfdChannel_->disableAll();
    timerfdChannel_->remove();
    ::close(timerfd_);
    for (const auto& entry : timers_)
    {
        delete entry.second;
    }
}

void TimerManager::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    TimerNode* timer = new TimerNode(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerManager::insert, this, timer));
}

bool TimerManager::insert(TimerNode* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    timers_.insert(Entry(when, timer));

    if (earliestChanged)
    {
        resetTimerfd(timerfd_, when);
    }
    return earliestChanged;
}

void TimerManager::handleRead()
{
    Timestamp now = steady_clock::now();
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    for (const auto& entry : expired)
    {
        entry.second->run();
    }

    reset(expired, now);
}

std::vector<TimerManager::Entry> TimerManager::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<TimerNode*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    return expired;
}

void TimerManager::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const auto& entry : expired)
    {
        if (entry.second->repeat())
        {
            entry.second->restart(now);
            insert(entry.second);
        }
        else
        {
            delete entry.second;
        }
    }

    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.time_since_epoch().count() > 0)
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}
