cmake_minimum_required(VERSION 3.10)
project(MyProject)

# set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# global source files variable
set(LOG_SOURCES
    ${CMAKE_SOURCE_DIR}/Log/src/AppendFile.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/AsyncLogging.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/FixedBuffer.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/LogFile.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/LoggingManager.cpp
    ${CMAKE_SOURCE_DIR}/Log/src/LogStream.cpp
)

set(WEBSERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/WebServer/src/Channel.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/Epoll.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/EventLoop.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/EventLoopThread.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/EventLoopThreadPool.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/HttpData.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/Server.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/Timer.cpp
    ${CMAKE_SOURCE_DIR}/WebServer/src/Util.cpp
)

set(WEBBENCH_SOURCES
    ${CMAKE_SOURCE_DIR}/WebBench/src/server.cpp
    ${CMAKE_SOURCE_DIR}/WebBench/src/webbench.cpp
)

# add subdirectory
add_subdirectory(Log)
add_subdirectory(WebServer)
add_subdirectory(WebBench)
add_subdirectory(Demo)
