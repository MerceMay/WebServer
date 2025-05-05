#include "LoggingManager.h"

LoggingManager LoggingManager::instance; // singleton instantiation

void LoggingManager::stopAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (asyncLoggingMap.empty())
    {
        return;
    }
    for (auto& [path, logger] : asyncLoggingMap)
    {
        logger->stop();
        logger.reset(); // reset the shared_ptr to release the resource
    }
    asyncLoggingMap.clear();
}

LoggingManager& LoggingManager::getInstance()
{
    return instance;
}

std::shared_ptr<AsyncLogging> LoggingManager::getAsyncLogging(const std::string& logFileName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = asyncLoggingMap.find(logFileName);
    if (it != asyncLoggingMap.end())
    {
        return it->second;
    }
    else
    {
        auto asyncLogging = std::make_shared<AsyncLogging>(logFileName);
        asyncLogging->start(); // start the AsyncLogging thread
        asyncLoggingMap[logFileName] = asyncLogging;
        return asyncLogging;
    }
}
