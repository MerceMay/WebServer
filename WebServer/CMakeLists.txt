# CMakeLists.txt for WebServer

# add the header file directory
include_directories(inc)
include_directories(${CMAKE_SOURCE_DIR}/Log/inc) # add the header file for Log

# generate the source files
add_library(WebServer STATIC ${WEBSERVER_SOURCES})

# link the Log library
target_link_libraries(WebServer Log)
