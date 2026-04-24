#include "distributed_cache_engine/utils/logger.h"

#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <unistd.h>

using namespace std;

namespace dce::utils
{
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::info(const std::string& message)
{
    log("INFO", message);
}

void Logger::warning(const std::string& message)
{
    log("WARN", message);
}

void Logger::error(const std::string& message)
{
    log("ERROR", message);
}

const char* Logger::colorForLevel(const string& level)
{
    if (level == "INFO")
    {
        return "\033[32m";
    }

    if (level == "WARN")
    {
        return "\033[33m";
    }

    if (level == "ERROR")
    {
        return "\033[31m";
    }

    return "\033[0m";
}

bool Logger::useColor()
{
    const char* disable_color = getenv("NO_COLOR");
    if (disable_color != nullptr)
    {
        return false;
    }

    return ::isatty(STDOUT_FILENO) != 0 || ::isatty(STDERR_FILENO) != 0;
}

void Logger::log(const std::string& level, const std::string& message)
{
    lock_guard<mutex> lock{mutex_};

    const auto now = chrono::system_clock::now();
    const time_t timestamp = chrono::system_clock::to_time_t(now);

    tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &timestamp);
#else
    localtime_r(&timestamp, &local_time);
#endif

    ostringstream output;
    output << "[" << put_time(&local_time, "%Y-%m-%d %H:%M:%S") << "] "
           << "[" << level << "] " << message << '\n';

    const string rendered = useColor()
        ? string(colorForLevel(level)) + output.str() + "\033[0m"
        : output.str();

    if (level == "ERROR")
    {
        cerr << rendered;
    }
    else
    {
        cout << rendered;
    }
}
} // namespace dce::utils
