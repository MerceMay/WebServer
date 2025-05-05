#include "server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int Socket(const char* host, int clientPort)
{
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;
    ad.sin_port = htons(clientPort);
    ad.sin_addr.s_addr = inet_addr(host);
    if (ad.sin_addr.s_addr == INADDR_NONE)
    {
        struct hostent* hp = gethostbyname(host);
        if (hp == nullptr)
            return -1;
        memcpy(&ad.sin_addr, hp->h_addr, hp->h_length);
    }

    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0)
        return socket;
    if (connect(socket, (struct sockaddr*)&ad, sizeof(ad)) < 0)
    {
        close(socket);
        return -1;
    }
    return socket;
}
