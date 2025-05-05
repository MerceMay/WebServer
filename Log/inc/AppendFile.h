#pragma once
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

class AppendFile
{
public:
    AppendFile(const AppendFile&) = delete;
    AppendFile& operator=(const AppendFile&) = delete;

    explicit AppendFile(const std::string& filename);
    ~AppendFile();

    void append(const char* logline, size_t len);
    void flush();

private:
    std::ofstream file_;
    std::vector<char> buffer_;
};