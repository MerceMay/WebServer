
void EventLoop::runAt(std::chrono::steady_clock::time_point time, std::function<void()> cb)
{
    timerQueue_->addTimer(cb, time, 0.0);
}

void EventLoop::runAfter(double delay, std::function<void()> cb)
{
    auto time = std::chrono::steady_clock::now() + std::chrono::microseconds(static_cast<int64_t>(delay * 1000000));
    runAt(time, cb);
}

void EventLoop::runEvery(double interval, std::function<void()> cb)
{
    auto time = std::chrono::steady_clock::now() + std::chrono::microseconds(static_cast<int64_t>(interval * 1000000));
    timerQueue_->addTimer(cb, time, interval);
}
