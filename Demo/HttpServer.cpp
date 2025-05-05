#include "EventLoop.h"
#include "Logger.h"
#include "Server.h"
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    int threadNum = std::thread::hardware_concurrency(); // get the number of hardware threads
    int port = 8080;

    int opt;
    const char* optString = "t:p:";
    while ((opt = getopt(argc, argv, optString)) != -1)
    {
        switch (opt)
        {
        case 't':
            threadNum = atoi(optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            break;
        }
    }    
    std::shared_ptr<EventLoop> mainEventLoop = std::make_shared<EventLoop>();
    mainEventLoop->initEventChannel(); // initialize the event channel
    Server myHTTPServer(mainEventLoop, threadNum, port);

    myHTTPServer.start();
    mainEventLoop->loop();
    return 0;
}
