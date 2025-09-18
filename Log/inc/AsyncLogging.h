
#pragma once

#include "FixedBuffer.h"
#include "LogFile.h"

#include <chrono>
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
    const std::chrono::seconds flushInterval_;
    const std::string logPath_;
    std::atomic<bool> isRunning_{false}; // initialized to false
    std::thread thread_;
    mutable std::mutex mutex_; // mutable to allow locking in const methods
    std::condition_variable cond_;
    std::unique_ptr<Buffer> currentBuffer_;
    std::unique_ptr<Buffer> backupBuffer_;
    std::vector<std::unique_ptr<Buffer>> buffers_;
    std::latch latch_;

    static constexpr size_t BufferVectorSize = 16; // preallocate space for 16 buffers

    void threadFunc();
    void swapBuffers(std::vector<std::unique_ptr<Buffer>>& buffersToWrite);

public:
    explicit AsyncLogging(const std::string& filename, int flushInterval = 3);
    ~AsyncLogging();

    // 禁用复制和移动
    AsyncLogging(const AsyncLogging&) = delete;
    AsyncLogging& operator=(const AsyncLogging&) = delete;
    AsyncLogging(AsyncLogging&&) = delete;
    AsyncLogging& operator=(AsyncLogging&&) = delete;

    void append(const char* logline, size_t len);
    void start();
    void stop();
};