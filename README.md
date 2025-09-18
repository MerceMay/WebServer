# WebServer

## Project Overview

This project is a high-performance Web Server inspired by [linyacool/WebServer](https://github.com/linyacool/WebServer). The implementation is rewritten using my preferred coding style while maintaining the same core logic. The project is written in C++ and aims to deliver efficient and reliable HTTP services. 

Additionally, the project includes:
- A **double-buffered, multi-threaded logging system** for high-performance logging.
- A lightweight benchmarking tool, **WebBench**, from [EZLippi/WebBench](https://github.com/EZLippi/WebBench).

---

## Features

- **Efficient I/O**: Uses epoll for I/O multiplexing.
- **Concurrency**: Multi-threaded model with thread pool support.
- **HTTP Support**: Handles HTTP request parsing and response generation.
- **Static Resource Serving**: Supports serving static files.
- **Logging System**:
  - Double-buffered for efficient I/O.
  - Asynchronous and multi-threaded to minimize performance bottlenecks.
- **Benchmarking Tool**: Includes WebBench for performance testing.

---

## Project Structure

```
.
├── bin                # Executables
│   ├── Client         # HTTP client
│   ├── Server         # HTTP server
│   └── WebBench       # Benchmarking tool
├── build              # Build directory
├── Demo               # Example usage of HTTP client and server
├── lib                # Static libraries for logging and the web server
├── Log                # Logging system implementation
├── Resource           # Static resources for testing
├── WebBench           # WebBench benchmarking tool implementation
└── WebServer          # Core web server implementation
```

---

## Build & Run

### Prerequisites

- Linux operating system
- C++20 or later
- CMake 3.10 or later

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/MerceMay/WebServer.git
   cd WebServer
   ```

2. Create and navigate to the build directory:
   ```bash
   mkdir build && cd build
   ```

3. Generate build configuration using CMake:
   ```bash
   cmake ..
   ```

4. Compile the project:
   ```bash
   make
   ```

### Run the Server

After the build is complete, you can navigate to the parent directory and start the server with:
```bash
cd .. && bin/Server -t <thread_number> -p <port>
```

Alternatively, to test the server:
1. Ensure you are in parent directory and execute the server:
   ```bash
   bin/Client
   ```
2. Or open your browser and visit:
   ```text
   http://localhost:<port>
   ```

---

## Logging System

The logging system is designed for high performance and scalability, with the following features:
1. **Double Buffering**:
   - Uses two buffers alternately. One buffer collects log entries while the other is being written to a file.
2. **Multi-threading**:
   - Dedicated threads handle logging to ensure minimal impact on server performance.
3. **Asynchronous Logging**:
   - Logs are written to files asynchronously, improving I/O efficiency and reducing blocking operations.

The logging system's implementation can be found in the `Log` directory:
- Header files: `Log/inc`
- Source files: `Log/src`

---

## Benchmarking Tool: WebBench

This project includes [WebBench](https://github.com/EZLippi/WebBench), a simple yet efficient benchmarking tool for testing the server's performance under load.

### Usage
Run the benchmarking tool with the desired parameters:
```bash
bin/WebBench -c <connections> -t <time> <url>
```
Example:
```bash
bin/WebBench -c 5 -t 5 http://localhost:<port>/
```
---

## Acknowledgments

- The web server and log system implementation is inspired by [linyacool/WebServer](https://github.com/linyacool/WebServer).
- The benchmarking tool is based on [EZLippi/WebBench](https://github.com/EZLippi/WebBench).

---

## License

This project is licensed under the [MIT License](LICENSE).