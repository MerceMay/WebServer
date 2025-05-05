#include "AsyncLogging.h"
#include <cassert>

AsyncLogging::AsyncLogging(const std::string& filename, int flushInterval)
    : flushInterval_(flushInterval),
      logPath_(filename),
      isRunning_(false),
      thread_(),
      mutex_(),
      cond_(),
      currentBuffer_(std::make_unique<Buffer>()),
      backupBuffer_(std::make_unique<Buffer>()),
      buffers_(),
      latch_(1), // initialize latch with 1
      logFile_(filename)
{
    assert(logPath_.size() > 0);
    currentBuffer_->zero();             // initialize current buffer
    backupBuffer_->zero();              // initialize backup buffer
    buffers_.reserve(BufferVectorSize); // pre-allocate buffer collection size
}

AsyncLogging::~AsyncLogging()
{
    if (isRunning_) // if the background thread is running, stop it
    {
        stop();
    }
}

void AsyncLogging::append(const char* logline, const size_t len)
{
    std::unique_lock<std::mutex> lock(mutex_); // lock the mutex to protect buffers_ and currentBuffer_

    if (currentBuffer_->available() > len) // if there is enough space in the current buffer
    {
        currentBuffer_->append(logline, len); // append log data to current buffer
    }
    else
    {                                                  // if there is not enough space in the current buffer
        buffers_.push_back(std::move(currentBuffer_)); // move current buffer to buffers_
        if (backupBuffer_)
        {                                              // if there is a backup buffer
            currentBuffer_ = std::move(backupBuffer_); // move backup buffer to current buffer
        }
        else
        {                                                // if there is no backup buffer
            currentBuffer_ = std::make_unique<Buffer>(); // create a new current buffer
        }
        currentBuffer_->append(logline, len); // append log data to current buffer
        cond_.notify_one();                   // notify the background thread to write data
    }
}

void AsyncLogging::start()
{
    if (!isRunning_) // if the background thread is not running
    {
        isRunning_ = true;
        thread_ = std::thread(&AsyncLogging::threadFunc, this);
        latch_.wait(); // wait for the background thread to be ready
    }
}

void AsyncLogging::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_); // lock the mutex to protect isRunning_
        isRunning_ = false;                        // set the background thread to stop running
        cond_.notify_all();
    }
    if (thread_.joinable()) // if the background thread is joinable, wait for it to finish
    {
        thread_.join();
    }
}

void AsyncLogging::threadFunc()
{
    latch_.count_down();                                 // count down the latch to indicate that the thread is ready
    std::vector<std::unique_ptr<Buffer>> buffersToWrite; // buffers for the background thread to write data
    buffersToWrite.reserve(BufferVectorSize);            // pre-allocate buffer size

    while (isRunning_ || !buffers_.empty()) // if the thread is running or there are unprocessed buffers
    {
        {
            std::unique_lock<std::mutex> lock(mutex_); // lock the mutex

            // if there is data in the current buffer, move it to buffers_
            if (currentBuffer_ && currentBuffer_->size() > 0)
            {
                buffers_.push_back(std::move(currentBuffer_));
                currentBuffer_ = std::make_unique<Buffer>(); // create a new current buffer
            }

            // if there is no backup buffer, create one
            if (!backupBuffer_)
            {
                backupBuffer_ = std::make_unique<Buffer>();
            }

            // if there is no new data and the thread is not terminating, wait for a while
            if (buffers_.empty() && isRunning_)
            {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }

            // ensure exit condition: if the thread has stopped and there is no data to process, exit the loop
            if (!isRunning_ && buffers_.empty())
            {
                break;
            }

            // move all buffers from buffers_ to buffersToWrite
            buffersToWrite.swap(buffers_);
        }
        // if the size of buffersToWrite exceeds the pre-allocated size, discard some buffers
        if (buffersToWrite.size() > BufferVectorSize)
        {
            buffersToWrite.resize(BufferVectorSize);
            // log a warning message
            logFile_.append("Warning: Log generation rate is too high.\n");
        }

        // write all buffers in buffersToWrite to the log file
        for (const auto& buffer : buffersToWrite)
        {
            logFile_.append(buffer->data(), buffer->size());
        }

        logFile_.flush();       // flush the log file to ensure all data is written
        buffersToWrite.clear(); // clear buffersToWrite for the next iteration
    }
    logFile_.flush(); // make sure to flush the log file before exiting
}