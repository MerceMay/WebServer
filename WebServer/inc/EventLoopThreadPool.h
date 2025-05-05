#include "EventLoopThread.h"
#include "Logger.h"

class EventLoopThreadPool
{
private:
    std::shared_ptr<EventLoop> mainEventLoop_;              // Base event loop
    bool started_;                                          // Whether the thread pool has started
    int numThreads_;                                        // Number of threads in the thread pool
    int nextIndex_;                                         // Index of the next thread
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // Threads in the thread pool
    std::vector<std::weak_ptr<EventLoop>> loops_;           // Event loops in the thread pool
public:
    EventLoopThreadPool(std::shared_ptr<EventLoop> mainLoop, int numThreads);
    ~EventLoopThreadPool() { LOG("log") << "EventLoopThreadPool destructor"; }
    
    EventLoopThreadPool() = delete;
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool(EventLoopThreadPool&&) = delete;
    EventLoopThreadPool& operator=(EventLoopThreadPool&&) = delete;
    void start(); // Start the thread pool

    std::weak_ptr<EventLoop> getNextLoop(); // Get the next event loop
};