#pragma once

#include "distributed_cache_engine/parser/command_parser.h"

namespace dce::parser
{
class Parser : public CommandParser
{
public:
    std::optional<core::Command> parse(const std::string& raw_request) const override;
};
} // namespace dce::parser
