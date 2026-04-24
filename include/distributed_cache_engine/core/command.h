#pragma once

#include <string>
#include <vector>

namespace dce::core
{
enum class CommandType
{
    Help,
    Ping,
    Set,
    SetEx,
    Get,
    Delete,
    Find,
    Exists,
    Unknown
};

struct Command
{
    CommandType type{CommandType::Unknown};
    std::vector<std::string> arguments;
    std::string raw;
};
} // namespace dce::core
