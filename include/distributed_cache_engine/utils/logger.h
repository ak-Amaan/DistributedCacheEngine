#pragma once

#include <mutex>
#include <string>

namespace dce::utils
{
class Logger
{
public:
    static Logger& instance();

    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    static const char* colorForLevel(const std::string& level);
    static bool useColor();
    Logger() = default;
    void log(const std::string& level, const std::string& message);

    std::mutex mutex_;
};
} // namespace dce::utils
