#include "Logger.h"

int main()
{
    LOG("app.log") << "This is a test log message.";
    LOG("error.log") << "This is an error log message.";

    std::cout << "Logging completed." << std::endl;
    return 0;
}