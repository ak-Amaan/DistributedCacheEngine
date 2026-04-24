#pragma once

#include "distributed_cache_engine/core/command.h"

#include <optional>
#include <string>

namespace dce::parser
{
class CommandParser
{
public:
    virtual ~CommandParser() = default;
    virtual std::optional<core::Command> parse(const std::string& raw_request) const = 0;
};
} // namespace dce::parser
