#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    char buffer[4096];
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    int sockfd = -1;
    while (true)
    {
        // retry creating socket and connecting
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("socket");
            sleep(5); // socket creation failed, wait 5 seconds before retrying
            continue;
        }

        std::cout << "Attempting to connect to server..." << std::endl;
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0)
        {
            std::cout << "Successfully connected to server." << std::endl;
            break; // successfully connected, exit the loop
        }
        else
        {
            perror("connect");
            close(sockfd); // close the socket on failure
            sleep(5);      // connection failed, wait 5 seconds before retrying
            continue;
        }
    }
    std::cout << "Connected to server." << std::endl;
    const char* request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nContent-Type: plain/text\r\nConnection: Keep-Alive\r\n\r\n";
    ssize_t bytes_sent;
    while (true)
    {
        bytes_sent = send(sockfd, request, strlen(request), 0);
        if (bytes_sent >= 0)
        {
            std::cout << "Sent " << bytes_sent << " bytes to server." << std::endl;
            break; // successfully sent, exit the loop
        }
        else
        {
            perror("send");
            if (errno == EPIPE || errno == ECONNRESET)
            {
                std::cerr << "Connection broken. Reconnecting..." << std::endl;
                close(sockfd);
                // retry creating socket and connecting
                sockfd = -1;
                break;
            }
            else if (errno == EINTR)
            {
                std::cout << "Send interrupted, retrying..." << std::endl;
                sleep(1);
                continue;
            }
            else
            {
                std::cerr << "Send error, retrying..." << std::endl;
                sleep(5); // other send error, wait 5 seconds before retrying
                continue;
            }
        }
    }

    if (sockfd != -1)
    {
        ssize_t bytes_received;
        while (true)
        {
            bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = '\0';
                std::cout << "Received " << bytes_received << " bytes from server: " << buffer << std::endl;
                break; // successfully received, exit the loop
            }
            else if (bytes_received == 0)
            {
                std::cout << "Server closed the connection. Reconnecting..." << std::endl;
                close(sockfd);
                sockfd = -1;
                break; // server closed the connection, exit the loop
            }
            else
            {
                perror("recv");
                if (errno == EINTR)
                {
                    std::cout << "Receive interrupted, retrying..." << std::endl;
                    sleep(10);
                    continue;
                }
                else if (errno == EPIPE || errno == ECONNRESET)
                {
                    std::cerr << "Connection broken during receive. Reconnecting..." << std::endl;
                    close(sockfd);
                    sockfd = -1;
                    break;
                }
                else
                {
                    std::cerr << "Receive error, retrying..." << std::endl;
                    sleep(5); // other receive error, wait 5 seconds before retrying
                    continue;
                }
            }
        }
        if (sockfd != -1)
        {
            close(sockfd); // close the socket if it was successfully created
        }
    }

    return 0;
}