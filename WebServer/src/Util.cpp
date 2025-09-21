#include "Util.h"
#include "Buffer.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const int MAX_BUFF = 4096;
ssize_t readn(int fd, void* buff, size_t n)
{
    size_t remain = n;
    ssize_t readed = 0;
    ssize_t read_sum = 0;
    char* ptr = static_cast<char*>(buff);
    while (remain > 0)
    {
        readed = read(fd, ptr + read_sum, remain);
        if (readed < 0)
        {
            if (errno == EINTR)
            {
                readed = 0;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return read_sum;
            }
            else
            {
                return -1;
            }
        }
        else if (readed == 0)
        {
            break;
        }
        read_sum += readed;
        remain -= readed;
        ptr += readed;
    }
    return read_sum;
}

ssize_t readn(int fd, std::string& inBuffer, bool& zero)
{
    ssize_t readed = 0;
    ssize_t read_sum = 0;
    while (true)
    {
        char buffer[MAX_BUFF];
        if ((readed = read(fd, buffer, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
            {
                readed = 0;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return read_sum;
            }
            else
            {
                return -1;
            }
        }
        else if (readed == 0)
        {
            zero = true;
            break;
        }
        read_sum += readed;
        inBuffer.append(buffer, readed);
    }
    return read_sum;
}
ssize_t readn(int fd, std::string& inBuffer)
{
    bool zero = false;
    return readn(fd, inBuffer, zero);
}

ssize_t readn(int fd, Buffer& inBuffer, bool& zero)
{
    ssize_t readed = 0;
    ssize_t read_sum = 0;
    while (true)
    {
        inBuffer.ensureWritableBytes(MAX_BUFF);
        char* buf = inBuffer.beginWrite();
        
        if ((readed = read(fd, buf, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
            {
                readed = 0;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return read_sum;
            }
            else
            {
                return -1;
            }
        }
        else if (readed == 0)
        {
            zero = true;
            break;
        }
        read_sum += readed;
        inBuffer.hasWritten(readed);
    }
    return read_sum;
}

ssize_t readn(int fd, Buffer& inBuffer)
{
    bool zero = false;
    return readn(fd, inBuffer, zero);
}

ssize_t writen(int fd, void* buff, size_t n)
{
    size_t remain = n;
    ssize_t writed = 0;
    ssize_t write_sum = 0;
    char* ptr = static_cast<char*>(buff);
    while (remain > 0)
    {
        writed = write(fd, ptr + write_sum, remain);
        if (writed < 0)
        {
            if (errno == EINTR)
            {
                writed = 0;
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return write_sum;
            }
            else
            {
                return -1;
            }
        }
        write_sum += writed;
        remain -= writed;
        ptr += writed;
    }
    return write_sum;
}
ssize_t writen(int fd, std::string& sbuff)
{
    size_t remain = sbuff.size();
    ssize_t writed = 0;
    ssize_t write_sum = 0;
    char* ptr = const_cast<char*>(sbuff.c_str());
    while (remain > 0)
    {
        writed = write(fd, ptr + write_sum, remain);
        if (writed < 0)
        {
            if (errno == EINTR)
            {
                writed = 0;
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                return -1;
            }
        }
        write_sum += writed;
        remain -= writed;
        ptr += writed;
    }
    if (write_sum == sbuff.size()) // write all
    {
        sbuff.clear();
    }
    else // write partial
    {
        sbuff = sbuff.substr(write_sum);
    }
    return write_sum;
}

ssize_t writen(int fd, Buffer& sbuff)
{
    size_t remain = sbuff.readableBytes();
    ssize_t writed = 0;
    ssize_t write_sum = 0;
    const char* ptr = sbuff.peek();
    
    while (remain > 0)
    {
        writed = write(fd, ptr + write_sum, remain);
        if (writed < 0)
        {
            if (errno == EINTR)
            {
                writed = 0;
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                return -1;
            }
        }
        write_sum += writed;
        remain -= writed;
    }
    
    // Remove written data from buffer
    if (write_sum > 0)
    {
        sbuff.retrieve(write_sum);
    }
    
    return write_sum;
}

// If one end closes the connection while the other end continues to write data to it,
// the first write will receive an RST response. Subsequent writes will cause the kernel
// to send a SIGPIPE signal to the process, notifying it that the connection has been broken.
// The default behavior of the SIGPIPE signal is to terminate the program.
void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN; // ignore SIGPIPE
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        perror("sigaction");
        return;
    }
}
int setSocketNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        return -1;
    }
    return 0;
}
void setSocketNodelay(int fd)
{
    int opt = 1;
    if (setsockopt(fd,
                   IPPROTO_TCP,
                   TCP_NODELAY,
                   (const char*)&opt,
                   sizeof(opt)) < 0) // set no delay
    {
        perror("setsockopt");
        return;
    }
}
void setSocketNoLinger(int fd)
{
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0)
    {
        perror("setsockopt");
        return;
    }
}
void shutDownWR(int fd)
{
    if (shutdown(fd, SHUT_WR) < 0)
    {
        perror("shutdown");
        return;
    }
}
int socket_bind_listen(int port)
{
    if (port < 0 || port > 65535)
    {
        return -1;
    }
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        return -1;
    }

    // set address reuse option
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0)
    {
        close(listenfd);
        return -1;
    }

    // set server ip and port
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(static_cast<uint16_t>(port));
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
    {
        close(listenfd);
        return -1;
    }
    // set the number of connections
    if (listen(listenfd, SOMAXCONN) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}