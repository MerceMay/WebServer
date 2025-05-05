# CMakeLists.txt for WebBench

# add the header file directory
include_directories(inc)

# add the executable target for WebBench
add_executable(WebBench ${WEBBENCH_SOURCES})