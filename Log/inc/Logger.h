#pragma once
#include "LogStream.h"
#include "LoggingManager.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

class Logger
{
public:
    explicit Logger(const char* filename, int line, const std::string& logPath);

    ~Logger();

    // return the log stream for appending log messages
    LogStream& getStream() { return m_stream; }

private:
    void formatTime();

    std::string m_filename; // current file name
    int m_line;             // current line number
    std::string m_logPath;  // log file path
    LogStream m_stream;     // log stream for appending log content
};

// 宏定义，简化日志调用
#define LOG(logPath) Logger(__FILE__, __LINE__, logPath).getStream()