# CMakeLists.txt for Log library

# add the header file directory
include_directories(inc)

# generate the static library
add_library(Log STATIC ${LOG_SOURCES})