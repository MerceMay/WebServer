#pragma once
#include "FixedBuffer.h"
#include "LogFile.h"
#include <condition_variable>
#include <latch>
#include <memory>
#include <string>
#include <thread>
#include <vector>
class AsyncLogging
{
private:
    using Buffer = FixedBuffer<kSmallBuffer>; // 4K

    LogFile logFile_;
    const int flushInterval_;
    const std::string logPath_;
    std::atomic<bool> isRunning_; // thread running flag
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::unique_ptr<Buffer> currentBuffer_;        // the buffer that is currently being written to by the front end.
    std::unique_ptr<Buffer> backupBuffer_;         // the reserve buffer that will be
    std::vector<std::unique_ptr<Buffer>> buffers_; // the collection of buffers that are waiting to be written to the log file.
    std::latch latch_;

    const size_t BufferVectorSize = 16; // the pre-allocated size of buffers_

    void threadFunc();

public:
    AsyncLogging(const std::string& filename, int flushInterval = 3);
    ~AsyncLogging();
    AsyncLogging(const AsyncLogging&) = delete;            // Disable copy constructor
    AsyncLogging& operator=(const AsyncLogging&) = delete; // Disable copy assignment
    AsyncLogging(AsyncLogging&&) = delete;                 // Disable move constructor
    AsyncLogging& operator=(AsyncLogging&&) = delete;      // Disable move assignment

    void append(const char* logline, const size_t len);
    void start();
    void stop();
};