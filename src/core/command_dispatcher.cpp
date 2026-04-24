#include "distributed_cache_engine/core/command_dispatcher.h"

using namespace std;

namespace dce::core
{
namespace
{
std::string frameResponse(std::string response)
{
    while (!response.empty() && (response.back() == '\n' || response.back() == '\r'))
    {
        response.pop_back();
    }

    response.push_back('\n');
    return response;
}
} // namespace

CommandDispatcher::CommandDispatcher(
    std::shared_ptr<parser::CommandParser> parser,
    CacheEngine& engine
)
    : parser_(std::move(parser)), engine_(engine)
{
}

std::string CommandDispatcher::dispatch(const std::string& raw_request) const
{
    const auto command = parser_->parse(raw_request);
    if (!command.has_value())
    {
        return frameResponse("ERROR Invalid command. Type HELP for supported commands.");
    }

    return frameResponse(engine_.execute(*command));
}
} // namespace dce::core
