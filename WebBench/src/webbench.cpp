#include <bits/this_thread_sleep.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <iostream>
#include <vector>

#include "server.h"

static void usage(void)
{
    std::cerr << "webbench [option]... URL\n"
                 "  -f|--force               Don't wait for reply from server.\n"
                 "  -r|--reload              Send reload request - Pragma: "
                 "no-cache.\n"
                 "  -t|--time <sec>          Run benchmark for <sec> seconds. "
                 "Default 30.\n"
                 "  -p|--proxy <server:port> Use proxy server for request.\n"
                 "  -c|--clients <n>         Run <n> HTTP clients at once. Default "
                 "one.\n"
                 "  -k|--keep                Keep-Alive\n"
                 "  -9|--http09              Use HTTP/0.9 style requests.\n"
                 "  -1|--http10              Use HTTP/1.0 protocol.\n"
                 "  -2|--http11              Use HTTP/1.1 protocol.\n"
                 "  --get                    Use GET request method.\n"
                 "  --head                   Use HEAD request method.\n"
                 "  --options                Use OPTIONS request method.\n"
                 "  --trace                  Use TRACE request method.\n"
                 "  -?|-h|--help             This information.\n"
                 "  -V|--version             Display program version.\n";
}

enum class HttpMethod
{
    METHOD_GET,
    METHOD_HEAD,
    METHOD_OPTIONS,
    METHOD_TRACE
};

#define PROGRAM_VERSION "1.5"
int force = 0;
int force_reload = 0;
int keep_alive = 0;
int method = static_cast<int>(HttpMethod::METHOD_GET);
static const struct option long_options[] = {
    {"force", no_argument, &force, 1},
    {"reload", no_argument, &force_reload, 1},
    {"time", required_argument, nullptr, 't'},
    {"help", no_argument, nullptr, '?'},
    {"http09", no_argument, nullptr, '9'},
    {"http10", no_argument, nullptr, '1'},
    {"http11", no_argument, nullptr, '2'},
    {"get", no_argument, &method, static_cast<int>(HttpMethod::METHOD_GET)},
    {"head", no_argument, &method, static_cast<int>(HttpMethod::METHOD_HEAD)},
    {"options", no_argument, &method, static_cast<int>(HttpMethod::METHOD_OPTIONS)},
    {"trace", no_argument, &method, static_cast<int>(HttpMethod::METHOD_TRACE)},
    {"version", no_argument, nullptr, 'V'},
    {"proxy", required_argument, nullptr, 'p'},
    {"clients", required_argument, nullptr, 'c'},
    {"keep", no_argument, &keep_alive, 1},
    {nullptr, 0, nullptr, 0}};

static void build_request(std::string url);
static int bench(void);
void benchcore(const std::string& host, const int port, const std::string& request);

int benchtime = 30;
int http10 = 1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
std::string proxyhost;
int proxyport = 80;
int clients = 1;
std::string request;
std::string host;
int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        usage();
        return 2;
    }
    int options_index = 0;
    size_t colonPos = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "912Vfrt:p:c:?hk", long_options, &options_index)) !=
           EOF)
    {
        switch (opt)
        {
        case 0:
        {
            // long option flag set
            break;
        }
        case 'f':
        {
            force = 1;
            break;
        }
        case 'r':
        {
            force_reload = 1;
            break;
        }
        case '9':
        {
            http10 = 0;
            break;
        }
        case '1':
        {
            http10 = 1;
            break;
        }
        case '2':
        {
            http10 = 2;
            break;
        }
        case 'V':
        {
            std::cout << "webbench v" << PROGRAM_VERSION << std::endl;
            exit(2);
            break;
        }
        case 't':
        {
            benchtime = atoi(optarg);
            break;
        }
        case 'k':
        {
            keep_alive = 1;
            break;
        }
        case 'p':
        {
            /* proxy server parsing server:port */
            proxyhost = optarg;
            size_t colonPos = proxyhost.rfind(':');
            if (colonPos == std::string::npos)
                // No port specified, use default
                break;
            if (colonPos == 0)
            {
                // No hostname specified
                std::cerr << "Error in option --proxy " << optarg << ": Missing hostname."
                          << std::endl;
                return 2;
            }
            if (colonPos == proxyhost.length() - 1)
            {
                // No port specified
                std::cerr << "Error in option --proxy " << optarg << ": Missing port." << std::endl;
                return 2;
            }

            // get the port
            std::string portStr = proxyhost.substr(colonPos + 1);

            // modify proxyhost to only contain the hostname part
            proxyhost = proxyhost.substr(0, colonPos);

            // convert the port string to an integer
            proxyport = std::stoi(portStr);
            break;
        }
        case ':':
        case 'h':
        case '?':
        {
            usage();
            return 2;
            break;
        }
        case 'c':
        {
            clients = atoi(optarg);
            break;
        }
        }
    }

    if (optind == argc)
    {
        std::cerr << "webbench: Missing URL!" << std::endl;
        usage();
        return 2;
    }

    if (clients < 1)
        clients = 1;
    if (benchtime < 1)
        benchtime = 30;

    /* Copyright */
    std::cout << "Webbench - Simple Web Benchmark " << PROGRAM_VERSION << "\n"
              << "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
              << std::endl;

    build_request(argv[optind]);

    std::cout << "Benchmarking: ";

    if (clients == 1)
        std::cout << "1 client";
    else
        std::cout << clients << " clients";
    std::cout << ", running " << benchtime << " sec";

    if (force)
        std::cout << ", early socket close";
    if (!proxyhost.empty())
        std::cout << ", via proxy server " << proxyhost << ":" << proxyport;
    if (force_reload)
        std::cout << ", forcing reload";

    std::cout << "." << std::endl;

    return bench();
}

void build_request(std::string url)
{
    host.clear();
    request.clear();

    if (force_reload && !proxyhost.empty() && http10 < 1)
        http10 = 1;
    if (method == static_cast<int>(HttpMethod::METHOD_HEAD) && http10 < 1)
        http10 = 1;
    if (method == static_cast<int>(HttpMethod::METHOD_OPTIONS) && http10 < 2)
        http10 = 2;
    if (method == static_cast<int>(HttpMethod::METHOD_TRACE) && http10 < 2)
        http10 = 2;

    switch (method)
    {
    default:
    case static_cast<int>(HttpMethod::METHOD_GET):
        request = "GET";
        break;
    case static_cast<int>(HttpMethod::METHOD_HEAD):
        request = "HEAD";
        break;
    case static_cast<int>(HttpMethod::METHOD_OPTIONS):
        request = "OPTIONS";
        break;
    case static_cast<int>(HttpMethod::METHOD_TRACE):
        request = "TRACE";
        break;
    }

    request += " ";
    // request == "GET " or "HEAD " or "OPTIONS " or "TRACE "

    // check parameters: url
    size_t pos = url.find("://");
    if (pos == std::string::npos)
    {
        std::cerr << url << ": is not a valid URL." << std::endl;
        exit(2);
    }
    if (url.length() > 1500)
    {
        std::cerr << "URL is too long." << std::endl;
        exit(2);
    }
    if (url.compare(0, 7, "http://") != 0)
    {
        std::cerr << "Only HTTP protocol is directly supported, set --proxy "
                     "for others."
                  << std::endl;
        exit(2);
    }

    // protocol/host delimiter
    pos += 3;
    if (url.find('/', pos) == std::string::npos)
    {
        std::cerr << "Invalid URL syntax - hostname doesn't end with '/'." << std::endl;
        exit(2);
    }
    if (proxyhost.empty()) // if no proxy is set
    {
        // if there is port in URL, use it
        size_t colonPos = url.find(':', pos);
        size_t slashPos = url.find('/', pos);
        if (colonPos != std::string::npos && colonPos < slashPos)
        {
            host = url.substr(pos, colonPos - pos);
            proxyport = std::stoi(url.substr(colonPos + 1, slashPos - colonPos - 1));
            if (proxyport < 1 || proxyport > 65535)
                proxyport = 80;
        }
        else
        {
            // no port in URL, use default
            host = url.substr(pos, slashPos - pos);
        }
    }
    else
    {
        // if proxy is set, use it, request == "GET url" or "HEAD url" or
        // "OPTIONS url" or "TRACE url"
        request += url;
    }

    if (http10 == 1)
        request += " HTTP/1.0";
    else if (http10 == 2)
        request += " HTTP/1.1";

    request += "\r\n";

    if (http10 > 0)
        request += "User-Agent: MerceMay's WebBench " + std::string(PROGRAM_VERSION) + "\r\n";
    if (proxyhost.empty() && http10 > 0)
        request += "Host: " + host + "\r\n";
    if (force_reload && !proxyhost.empty())
        request += "Pragma: no-cache\r\n";
    if (http10 > 1)
    {
        if (keep_alive == 0)
            request += "Connection: close\r\n";
        else
            request += "Connection: Keep-Alive\r\n";
    }

    // add empty line at end
    if (http10 > 0)
        request += "\r\n";

    std::cout << "Request: " << request << std::endl;
}

static int speed = 0;
static int failed = 0;
static int bytes = 0;
int bench(void)
{
    // test if server is alive
    int socket = Socket(proxyhost.empty() ? host.c_str() : proxyhost.c_str(), proxyport);
    if (socket < 0)
    {
        std::cerr << "Connect to server failed. Aborting benchmark." << std::endl;
        return 1;
    }
    close(socket);

    int mypipe[2];
    if (pipe(mypipe) < 0)
    {
        std::cerr << "pipe failed." << std::endl;
        return 3;
    }

    std::vector<pid_t> pids(clients);
    int clientcounts = 0;
    for (clientcounts = 0; clientcounts < clients; clientcounts++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            std::cerr << "fork failed for client " << clientcounts << std::endl;
            sleep(1);
            break;
        }
        else if (pid == 0)
        {
            // child process
            close(mypipe[0]); // close read end
            if (proxyhost.empty())
            {
                benchcore(host, proxyport, request);
            }
            else
            {
                benchcore(proxyhost, proxyport, request);
            }
            FILE* fp = fdopen(mypipe[1], "w");
            if (fp == nullptr)
            {
                std::cerr << "open pipe for writing failed." << std::endl;
                close(mypipe[1]);
                return 3;
            }
            if (fprintf(fp, "%d %d %d\n", speed, failed, bytes) < 0)
            {
                std::cerr << "write to pipe failed." << std::endl;
                fclose(fp); // this will close mypipe[1]
                return 3;
            }
            fclose(fp); // this will close mypipe[1]
            return 0;
        }
        else
        {
            // parent process records child PID
            pids[clientcounts] = pid;
        }
    }
    // parent process collects results
    speed = 0;
    failed = 0;
    bytes = 0;
    int exited_children = 0;
    close(mypipe[1]); // close write end
    while (exited_children < clientcounts)
    {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0)
        {
            exited_children++;
        }
        else if (pid == -1 && errno == ECHILD)
        {
            break;
        }
    }

    FILE* fp = fdopen(mypipe[0], "r");
    if (fp == nullptr)
    {
        std::cerr << "open pipe for reading failed." << std::endl;
        close(mypipe[0]);
        return 3;
    }
    int total_speed = 0;
    int total_failed = 0;
    int total_bytes = 0;

    setvbuf(fp, nullptr, _IONBF, 0); // set no buffering

    for (int i = 0; i < clientcounts; i++)
    {
        int speed_result = 0;
        int failed_result = 0;
        int bytes_result = 0;

        // set read timeout
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(fileno(fp), &readfds);
        tv.tv_sec = static_cast<long>(benchtime); // set timeout to benchtime
        tv.tv_usec = 0;

        int selectResult = select(fileno(fp) + 1, &readfds, NULL, NULL, &tv);
        if (selectResult <= 0)
        {
            // timeout or error
            std::cerr << "Timeout or error reading from client " << i << std::endl;
            fclose(fp);
            continue;
        }

        if (fscanf(fp, "%d %d %d", &speed_result, &failed_result, &bytes_result) != 3)
        {
            std::cerr << "read incomplete data from client " << i << std::endl;
            fclose(fp);
            continue; // skip instead of fail
        }

        total_speed += speed_result;
        total_failed += failed_result;
        total_bytes += bytes_result;
    }
    fclose(fp); // close pipe read end
    // wait for all child processes to finish (although already waited above, this ensures all child processes have exited)
    for (int i = 0; i < clientcounts; i++)
    {
        int status;
        if (waitpid(pids[i], &status, WNOHANG) == 0)
        {
            // if child process is still running, force terminate
            kill(pids[i], SIGTERM);
            waitpid(pids[i], &status, 0);
        }
    }
    // update global variables
    speed = total_speed;
    failed = total_failed;
    bytes = total_bytes;

    std::cout << "\nSpeed=" << static_cast<int>(speed + failed) / (benchtime / 60.0f)
              << " pages/min, " << static_cast<int>((bytes) / static_cast<double>(benchtime))
              << " bytes/sec.\nRequests: " << speed << " succeed, " << failed << " failed."
              << std::endl;

    return 0;
}

volatile int timeout = 0;
void benchcore(const std::string& host, int port, const std::string& request)
{
    // setup alarm signal handler
    struct sigaction sa;
    sa.sa_handler = [](int signal)
    { timeout = 1; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, nullptr) == -1)
    {
        std::cerr << "sigaction failed." << std::endl;
        exit(3);
    }

    alarm(benchtime);

    if (keep_alive == 1)
    {
        int socket = Socket(host.c_str(), port);
        if (socket < 0)
        {
            std::cerr << "Initial keep-alive connection failed." << std::endl;
            failed++;
            return;
        }

        while (true)
        {
            if (timeout)
            {
                return;
            }

            // send request
            ssize_t sent = send(socket, request.c_str(), request.length(), 0);
            if (sent != static_cast<ssize_t>(request.length()))
            {
                failed++;
                close(socket);
                socket = Socket(host.c_str(), port);
                if (socket < 0)
                {
                    failed++;
                    continue; // fialed to reconnect, try next request
                }
                continue;
            }

            // If not in force mode, need to wait for a response
            if (force == 0)
            {
                bool recv_error = false;
                while (!timeout)
                {
                    char buffer[1500];
                    ssize_t recv_bytes = recv(socket, buffer, sizeof(buffer), 0);
                    if (recv_bytes < 0)
                    {
                        failed++;
                        recv_error = true;
                        close(socket);
                        socket = Socket(host.c_str(), port);
                        if (socket < 0)
                        {
                            failed++;
                            break;
                        }
                        break; // if failed to receive, try next request
                    }
                    else if (recv_bytes == 0)
                    {
                        // server closed connection
                        close(socket);
                        socket = Socket(host.c_str(), port);
                        if (socket < 0)
                        {
                            failed++;
                        }
                        break;
                    }
                    else
                    {
                        bytes += recv_bytes; // successfully received data
                        break;
                    }
                }

                if (recv_error || timeout)
                {
                    continue;
                }
            }

            // successfully sent request and received response
            speed++;
        }
    }
    else
    {
        while (true)
        {
            if (timeout)
            {
                if (failed > 0) // a request was terminated cause of timeout
                {
                    failed--;
                }
                return;
            }

            int socket = Socket(host.c_str(), port);
            if (socket < 0)
            {
                failed++;
                continue;
            }

            if (send(socket, request.c_str(), request.length(), 0) !=
                request.length()) // failed to send request to server
            {
                failed++;
                close(socket);
                continue;
            }

            if (http10 == 0)
            {
                if (shutdown(socket, SHUT_WR) < 0)
                {
                    failed++;
                    close(socket);
                    continue;
                }
            }

            if (force == 0)
            {
                bool read_error = false;
                while (!read_error && !timeout)
                {
                    char buffer[1500];
                    int recv_bytes = recv(socket, buffer, sizeof(buffer), 0);
                    if (recv_bytes < 0)
                    {
                        failed++;
                        close(socket);
                        read_error = true;
                    }
                    else if (recv_bytes == 0)
                    {
                        break;
                    }
                    else
                    {
                        bytes += recv_bytes;
                        if (recv_bytes < 1500)
                        {
                            break; // end of response
                        }
                    }
                }
                if (read_error)
                {
                    continue;
                }
            }
            if (close(socket) < 0)
            {
                failed++;
                continue;
            }
            speed++;
        }
    }
}