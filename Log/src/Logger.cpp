#include "Logger.h"

Logger::Logger(const char* filename, int line, const std::string& logPath)
    : m_filename(filename), m_line(line), m_logPath(logPath)
{
    formatTime(); // format the current time
}

Logger::~Logger()
{
    // add file name and line number to the end of the log message
    m_stream << " -- " << m_filename << ":" << m_line << "\n";

    // send the log message to the async logging manager which will handle the actual writing to the log file
    const LogStream::Buffer& buffer = m_stream.buffer();
    LoggingManager::getInstance().getAsyncLogging(m_logPath)->append(buffer.data(), buffer.size());
}

void Logger::formatTime()
{
    // Get the current time and format it as a string
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // std::tm local_time = *std::localtime(&now_time);
    std::tm local_time;
    localtime_r(&now_time, &local_time); // thread-safe localtime
    std::ostringstream oss;
    oss << "[" << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << "] ";
    m_stream << oss.str(); // write timestamp to log stream
}
