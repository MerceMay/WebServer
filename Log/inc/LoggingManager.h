#pragma once
#include "AsyncLogging.h"
#include <unordered_map>

class LoggingManager
{
private:
    LoggingManager() = default;
    ~LoggingManager() { stopAll(); }
    LoggingManager(const LoggingManager&) = delete;            // Disable copy constructor
    LoggingManager& operator=(const LoggingManager&) = delete; // Disable copy assignment

    std::unordered_map<std::string, std::shared_ptr<AsyncLogging>> asyncLoggingMap; // manage AsyncLogging instances
    std::mutex mutex_;                                                              // mutex for thread safety

public:
    void stopAll(); // stop all AsyncLogging instances

    // get the singleton instance of LoggingManager
    static LoggingManager& getInstance();

    static LoggingManager instance; // singleton instance

    std::shared_ptr<AsyncLogging> getAsyncLogging(const std::string& logFileName);
};
