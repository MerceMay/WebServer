# CMakeLists.txt for Demo

# add the header file directory
include_directories(${CMAKE_SOURCE_DIR}/WebServer/inc)
include_directories(${CMAKE_SOURCE_DIR}/Log/inc)

# generate Server executable file
add_executable(Server HttpServer.cpp)

# link the WebServer library
target_link_libraries(Server WebServer)

# generate Client executable file
add_executable(Client HttpClient.cpp)
