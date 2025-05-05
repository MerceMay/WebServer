#pragma once
// #include "Epoll.h"
#include "Channel.h"
#include "Epoll.h"
#include "Logger.h"
#include "Util.h"
#include <cassert>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>
#include <vector>

class Epoll;
class Channel;

class EventLoop : public std::enable_shared_from_this<EventLoop>
{
private:
    bool isLooping_;  // loop flag
    bool shouldStop_; // stop flag

    std::shared_ptr<Epoll> epoll_; // epoll instance

    int eventFd_;                           // event file descriptor of eventfd for wakeup
    std::shared_ptr<Channel> eventChannel_; // wakeup channel

    std::vector<std::function<void()>> pendingFunctions_; // functions to be executed
    bool isCallingPendingFunctions_;                      // process pending functions flag
    void doPendingFunctions();                            // execute pending functions

    const std::thread::id threadId_; // thread ID of the current thread

    std::mutex mutex_; // mutex for thread safety

    bool isHandlingEvent_; // handling event flag

    void wakeup();        // wake up the event loop
    void handleRead();    // handle read event
    void handleConnect(); // handle connect event

public:
    EventLoop();
    void initEventChannel(); // initialize event channel
    ~EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    bool isEventLoopInOwnThread(); // assert if the event loop is in the current thread

    void loop(); // start the event loop

    void quit(); // stop the event loop

    void runInLoop(std::function<void()> func); // execute function in the event loop

    void queueInLoop(std::function<void()> func); // queue function in the event loop

    void shutdown(std::shared_ptr<Channel> request); // shutdown the channel

    void removeChannel(std::shared_ptr<Channel> request); // remove channel from epoll

    void addChannel(std::shared_ptr<Channel> request, int timeout = 0); // add channel to epoll

    void modChannel(std::shared_ptr<Channel> request, int timeout); // mod the event and timeout of event in epoll

    void modChannel(std::shared_ptr<Channel> request); // mod the event of event in epoll
};