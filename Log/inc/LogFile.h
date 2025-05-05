#pragma once
#include "AppendFile.h"
#include <memory>
#include <string>

class LogFile
{
public:
    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;
    LogFile(LogFile&&) = delete;
    LogFile& operator=(LogFile&&) = delete;

    LogFile(const std::string& filename, int flushLines = 1024);
    ~LogFile();

    void append(const char* logline, const ssize_t len);
    void append(const std::string& logline);

    void flush();

private:
    void append_unlocked(const char* logline, const ssize_t len);

    std::string filename_;
    int flushLines_;
    ssize_t count_;
    std::mutex mutex_;
    AppendFile file_;
};