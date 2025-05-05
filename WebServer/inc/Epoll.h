#pragma once

#include "HttpData.h"
#include "Logger.h"
#include <memory>
#include <sys/epoll.h>
#include <vector>

class Channel;
class HttpData;
class TimerManager;

class Epoll
{
private:
    static const int MAXFDS = 4096;                    // maximum number of file descriptors
    static const int EPOLL_TIMEOUT = 10000;            // epoll timeout in milliseconds
    int epollFd_;                                      // epoll instance file descriptor
    std::vector<struct epoll_event> events_;           // epoll events
    std::vector<std::shared_ptr<Channel>> channels_;   // the channel of the event 
    std::vector<std::shared_ptr<HttpData>> httpDatas_; // http data associated with channels
    std::unique_ptr<TimerManager> timerManager_;       // timer manager for handling timeouts
public:
    Epoll();
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;
    Epoll(Epoll&&) = delete;
    Epoll& operator=(Epoll&&) = delete;

    ~Epoll() {}

    bool add_epoll(std::shared_ptr<Channel> request, int timeout); // add the channel to epoll

    bool mod_epoll(std::shared_ptr<Channel> request, int timeout); // mod the event and timeout of the channel

    bool mod_epoll(std::shared_ptr<Channel> request); // mod the event of the channel

    bool del_epoll(std::shared_ptr<Channel> request); // delete the channel from epoll

    bool addTimerNode(std::shared_ptr<Channel> request, int timeout); // set the timeout of the channel

    std::vector<std::shared_ptr<Channel>> runEpoll(); // get the events that occurred in epoll by epoll_wait

    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_sum); // get the events that occurred in epoll

    void handleExpired(); // handle expired events

    int getEpollFd() const; // get the epoll file descriptor
};