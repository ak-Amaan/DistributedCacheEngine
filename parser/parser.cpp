#include "parser/parser.hpp"

#include "distributed_cache_engine/utils/string_utils.h"

#include <cctype>
#include <sstream>

using namespace std;

namespace dce::parser
{
namespace
{
optional<core::Command> buildGetOrDeleteCommand(
    core::CommandType type,
    const vector<string>& tokens,
    const string& raw_request
)
{
    if (tokens.size() != 2U)
    {
        return nullopt;
    }

    core::Command command;
    command.type = type;
    command.raw = raw_request;
    command.arguments.push_back(tokens[1]);
    return command;
}

optional<core::Command> buildFindCommand(
    const vector<string>& tokens,
    const string& raw_request
)
{
    if (tokens.size() != 5U)
    {
        return nullopt;
    }

    if (dce::utils::toUpper(tokens[1]) != "WHERE" || dce::utils::toUpper(tokens[2]) != "VALUE")
    {
        return nullopt;
    }

    if (tokens[3] != ">" && tokens[3] != "<" && tokens[3] != "==")
    {
        return nullopt;
    }

    core::Command command;
    command.type = core::CommandType::Find;
    command.raw = raw_request;
    command.arguments.push_back(tokens[3]);
    command.arguments.push_back(tokens[4]);
    return command;
}

bool isPositiveInteger(const string& value)
{
    if (value.empty())
    {
        return false;
    }

    for (const char character : value)
    {
        if (!isdigit(static_cast<unsigned char>(character)))
        {
            return false;
        }
    }

    return value != "0";
}
} // namespace

optional<core::Command> Parser::parse(const string& raw_request) const
{
    const string request = dce::utils::sanitizeCommandInput(raw_request);
    if (request.empty())
    {
        return nullopt;
    }

    const vector<string> tokens = dce::utils::splitWhitespace(request);
    if (tokens.empty())
    {
        return nullopt;
    }

    core::Command command;
    command.raw = request;

    const string verb = dce::utils::toUpper(tokens.front());
    if (verb == "HELP")
    {
        if (tokens.size() != 1U)
        {
            return nullopt;
        }

        command.type = core::CommandType::Help;
        return command;
    }

    if (verb == "PING")
    {
        if (tokens.size() != 1U)
        {
            return nullopt;
        }

        command.type = core::CommandType::Ping;
        return command;
    }

    if (verb == "SET")
    {
        if (tokens.size() < 3U)
        {
            return nullopt;
        }

        command.type = core::CommandType::Set;
        command.arguments.push_back(tokens[1]);

        const size_t value_start = request.find(tokens[2], request.find(tokens[1]) + tokens[1].size());
        if (value_start == string::npos)
        {
            return nullopt;
        }

        command.arguments.push_back(request.substr(value_start));
        return command;
    }

    if (verb == "GET")
    {
        return buildGetOrDeleteCommand(core::CommandType::Get, tokens, request);
    }

    if (verb == "DEL")
    {
        return buildGetOrDeleteCommand(core::CommandType::Delete, tokens, request);
    }

    if (verb == "SETEX")
    {
        if (tokens.size() < 4U || !isPositiveInteger(tokens[2]))
        {
            return nullopt;
        }

        command.type = core::CommandType::SetEx;
        command.arguments.push_back(tokens[1]);
        command.arguments.push_back(tokens[2]);

        const size_t value_start = request.find(tokens[3], request.find(tokens[2]) + tokens[2].size());
        if (value_start == string::npos)
        {
            return nullopt;
        }

        command.arguments.push_back(request.substr(value_start));
        return command;
    }

    if (verb == "FIND")
    {
        return buildFindCommand(tokens, request);
    }

    return nullopt;
}
} // namespace dce::parser
