#include "AsyncLogging.h"
#include <algorithm>
#include <cassert>

AsyncLogging::AsyncLogging(const std::string& filename, int flushInterval)
    : logFile_(filename),
      flushInterval_(std::chrono::seconds(flushInterval)),
      logPath_(filename),
      currentBuffer_(std::make_unique<Buffer>()),
      backupBuffer_(std::make_unique<Buffer>()),
      latch_(1)
{
    assert(!logPath_.empty());

    currentBuffer_->zero();
    backupBuffer_->zero();
    buffers_.reserve(BufferVectorSize);
}

AsyncLogging::~AsyncLogging()
{
    if (isRunning_.load())
    {
        stop();
    }
}

void AsyncLogging::append(const char* logline, size_t len)
{
    if (!logline || len == 0)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    if (currentBuffer_->available() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.emplace_back(std::move(currentBuffer_));

        if (backupBuffer_)
        {
            currentBuffer_ = std::move(backupBuffer_);
        }
        else
        {
            currentBuffer_ = std::make_unique<Buffer>();
        }

        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogging::start()
{
    bool expected = false;
    if (isRunning_.compare_exchange_strong(expected, true))
    {
        thread_ = std::thread(&AsyncLogging::threadFunc, this);
        latch_.wait();
    }
}

void AsyncLogging::stop()
{
    bool expected = true;
    if (isRunning_.compare_exchange_strong(expected, false))
    {
        cond_.notify_all();

        if (thread_.joinable())
        {
            thread_.join();
        }
    }
}

void AsyncLogging::swapBuffers(std::vector<std::unique_ptr<Buffer>>& buffersToWrite)
{
    // if current buffer has data, move it to buffers_
    if (currentBuffer_ && currentBuffer_->size() > 0)
    {
        buffers_.emplace_back(std::move(currentBuffer_));
        currentBuffer_ = std::make_unique<Buffer>();
    }

    // ensure there is a backup buffer
    if (!backupBuffer_)
    {
        backupBuffer_ = std::make_unique<Buffer>();
    }

    // swap the buffers
    buffersToWrite.swap(buffers_);
}

void AsyncLogging::threadFunc()
{
    latch_.count_down();

    std::vector<std::unique_ptr<Buffer>> buffersToWrite;
    buffersToWrite.reserve(BufferVectorSize);

    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);

            swapBuffers(buffersToWrite);

            // if no data to write, wait for notification or timeout
            if (buffersToWrite.empty() && isRunning_.load())
            {
                cond_.wait_for(lock, flushInterval_);
                swapBuffers(buffersToWrite); // recheck after waking up
            }

            // exit condition
            if (!isRunning_.load() && buffersToWrite.empty())
            {
                break;
            }
        }

        // if too many buffers, drop some
        if (buffersToWrite.size() > BufferVectorSize)
        {
            // keep only the most recent buffers
            std::rotate(buffersToWrite.begin(),
                        buffersToWrite.end() - BufferVectorSize,
                        buffersToWrite.end());
            buffersToWrite.resize(BufferVectorSize);

            const char* warning = "Warning: Log generation rate too high, dropping old logs\n";
            logFile_.append(warning, std::strlen(warning));
        }

        // write all buffers to log file
        for (const auto& buffer : buffersToWrite)
        {
            if (buffer && buffer->size() > 0)
            {
                logFile_.append(buffer->data(), buffer->size());
            }
        }

        if (!buffersToWrite.empty())
        {
            logFile_.flush();
            buffersToWrite.clear();
        }
    }

    // final flush before exiting
    logFile_.flush();
}