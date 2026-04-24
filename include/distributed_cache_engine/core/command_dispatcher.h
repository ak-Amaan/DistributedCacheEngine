#pragma once

#include "distributed_cache_engine/core/cache_engine.h"
#include "distributed_cache_engine/parser/command_parser.h"

#include <memory>
#include <string>

namespace dce::core
{
class CommandDispatcher
{
public:
    CommandDispatcher(std::shared_ptr<parser::CommandParser> parser, CacheEngine& engine);

    std::string dispatch(const std::string& raw_request) const;

private:
    std::shared_ptr<parser::CommandParser> parser_;
    CacheEngine& engine_;
};
} // namespace dce::core
