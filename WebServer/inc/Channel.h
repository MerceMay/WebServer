#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;

// Channel 不再持有任何业务对象的指针，只负责 fd 与 EventLoop 的交互
class Channel
{
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // Non-copyable, Non-movable (Channel belongs to a specific Loop/Connection)
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    void handleEvent();

    // Callbacks setup
    void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    int fd() const { return fd_; }
    int events() const { return events_; }
    
    // API for EventLoop interactions
    void setRevents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // For Epoll
    int index() { return index_; }
    void setIndex(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }
    
    // Tie this channel to the owner object (TcpConnection) to prevent use-after-free
    void tie(const std::shared_ptr<void>& obj);

private:
    void update(); // Calls loop_->updateChannel(this)
    void handleEventWithGuard();

    static const int kNoneEvent = 0;
    static const int kReadEvent = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;

    EventLoop* loop_; // Raw pointer: Loop outlives Channel
    const int fd_;
    int events_;
    int revents_;
    int index_; // Used by Poller
    
    bool tied_;
    std::weak_ptr<void> tie_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
