#include "LogFile.h"

LogFile::LogFile(const std::string& filename, int flushLines)
    : filename_(filename),
      flushLines_(flushLines),
      count_(0),
      file_(filename_)
{
}

LogFile::~LogFile()
{
    file_.flush();
}

void LogFile::append(const char* logline, const ssize_t len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    append_unlocked(logline, len);
}

void LogFile::append(const std::string& logline)
{
    std::lock_guard<std::mutex> lock(mutex_);
    append_unlocked(logline.c_str(), logline.size());
}

void LogFile::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    file_.flush();
}

void LogFile::append_unlocked(const char* logline, const ssize_t len)
{
    file_.append(logline, len);
    if (count_++ % flushLines_ == 0)
    {
        file_.flush();
    }
}
