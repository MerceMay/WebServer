#pragma once
#include <string>

class Buffer; // Forward declaration

ssize_t readn(int fd, void* buff, size_t n);
ssize_t readn(int fd, std::string& inBuffer, bool& zero);
ssize_t readn(int fd, std::string& inBuffer);
ssize_t readn(int fd, Buffer& inBuffer, bool& zero); // New function for Buffer
ssize_t readn(int fd, Buffer& inBuffer); // New function for Buffer
ssize_t writen(int fd, void* buff, size_t n);
ssize_t writen(int fd, std::string& sbuff);
ssize_t writen(int fd, Buffer& sbuff); // New function for Buffer
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd);
void shutDownWR(int fd);
int socket_bind_listen(int port);
