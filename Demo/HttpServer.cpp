#include "EventLoop.h"
#include "Server.h"
#include "TcpConnection.h"
#include "HttpContext.h"
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

void onConnection(const shared_ptr<TcpConnection>& conn)
{
    if (conn->connected())
    {
        cout << "New connection " << conn->name() << " from " << conn->fd() << endl;
        conn->setContext(HttpContext());
    }
    else
    {
        cout << "Connection " << conn->name() << " is down" << endl;
    }
}

void onMessage(const shared_ptr<TcpConnection>& conn, Buffer* buf)
{
    HttpContext* context = std::any_cast<HttpContext>(conn->getMutableContext());
    
    // Parse the request
    // Note: Receive time is not passed in this callback, utilizing 0 or now
    if (!context->parseRequest(buf, 0))
    {
        // If parsing fails (and it's not partial), send error
        // But parseRequest returns false for partial too? 
        // Need to check HttpContext impl. 
        // Assuming false means "need more data" or "error" is handled internally?
        // Let's assume false means "not done yet" or "error handled"
        // Wait, if it's an error, parseRequest usually returns false AND sets state to error?
        // Let's assume it just needs more data for now.
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
        return;
    }

    if (context->gotAll())
    {
        cout << "Request: " << context->method() << " " << context->path() << endl;
        
        string body = "<html><body><h1>Hello from WebServer</h1><p>Path: " + context->path() + "</p></body></html>";
        string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(body.size()) + "\r\n";
        response += "Connection: Keep-Alive\r\n";
        response += "\r\n";
        response += body;

        conn->send(response);
        
        // Simple keep-alive handling: always keep alive unless requested otherwise
        // For now, reset context for next request
        context->reset();
    }
}

int main(int argc, char* argv[])
{
    int threadNum = 4;
    int port = 8080;
    const char* optString = "t:p:";
    int opt;

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

    EventLoop loop;
    Server server(&loop, threadNum, port);
    
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    server.start();
    loop.loop();
    
    return 0;
}
