#include "AppendFile.h"

AppendFile::AppendFile(const std::string& filename)
    : file_(filename, std::ios::app | std::ios::out)
{
    if (!file_.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        throw std::runtime_error("Failed to open file: " + filename);
    }
    buffer_.resize(16 * 1024); // 16KB buffer
    file_.rdbuf()->pubsetbuf(buffer_.data(), buffer_.size());
}

AppendFile::~AppendFile()
{
    if (file_.is_open())
    {
        file_.close();
    }
}

void AppendFile::append(const char* logline, size_t len)
{
    file_.write(logline, len);
    if (file_.fail())
    {
        std::cerr << "AppendFile::append() failed." << std::endl;
        throw std::runtime_error("AppendFile::append() failed");
    }
}

void AppendFile::flush()
{
    file_.flush();
}
