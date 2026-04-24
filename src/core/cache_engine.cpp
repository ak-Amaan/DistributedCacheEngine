#include "distributed_cache_engine/core/cache_engine.h"

#include "persistence/aof_persistence.hpp"

#include <chrono>
#include <limits>
#include <sstream>
#include <vector>

using namespace std;

namespace dce::core
{
namespace
{
string okResponse(const string& message)
{
    return "OK " + message + "\n";
}

string errorResponse(const string& message)
{
    return "ERROR " + message + "\n";
}

string valueResponse(const string& message)
{
    return "VALUE " + message + "\n";
}

optional<storage::QueryOperator> parseQueryOperator(const string& value)
{
    if (value == ">")
    {
        return storage::QueryOperator::GreaterThan;
    }

    if (value == "<")
    {
        return storage::QueryOperator::LessThan;
    }

    if (value == "==")
    {
        return storage::QueryOperator::Equal;
    }

    return nullopt;
}

string formatArrayResponse(const vector<string>& values)
{
    if (values.empty())
    {
        return valueResponse("(empty)");
    }

    ostringstream response;
    response << "VALUE ";
    for (size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0U)
        {
            response << ", ";
        }
        response << values[index];
    }
    response << '\n';

    return response.str();
}

string helpResponse()
{
    return
        "OK Available commands:\n"
        "  HELP\n"
        "  PING\n"
        "  SET <key> <value>\n"
        "  GET <key>\n"
        "  DEL <key>\n"
        "  SETEX <key> <ttl_seconds> <value>\n"
        "  FIND WHERE value > <number>\n"
        "  FIND WHERE value < <number>\n"
        "  FIND WHERE value == <number>\n";
}
} // namespace

CacheEngine::CacheEngine(
    std::shared_ptr<storage::StorageEngine> storage,
    std::shared_ptr<persistence::AofPersistence> persistence
)
    : storage_(std::move(storage)), persistence_(std::move(persistence))
{
}

std::string CacheEngine::execute(const Command& command)
{
    switch (command.type)
    {
    case CommandType::Help:
        return helpResponse();
    case CommandType::Ping:
        return okResponse("PONG");
    case CommandType::Set:
        if (command.arguments.size() < 2U)
        {
            return errorResponse("SET requires <key> <value>");
        }

        if (!persistMutation(command))
        {
            return errorResponse("persistence failure");
        }

        storage_->set(command.arguments[0], command.arguments[1]);
        return "OK\n";
    case CommandType::SetEx:
        if (command.arguments.size() < 3U)
        {
            return errorResponse("SETEX requires <key> <ttl> <value>");
        }

        try
        {
            const long long ttl_value = stoll(command.arguments[1]);
            if (ttl_value <= 0 || ttl_value > numeric_limits<int>::max())
            {
                return errorResponse("SETEX ttl must be a positive integer");
            }

            if (!persistMutation(command))
            {
                return errorResponse("persistence failure");
            }

            storage_->set(
                command.arguments[0],
                command.arguments[2],
                chrono::seconds(ttl_value)
            );
            return "OK\n";
        }
        catch (const exception&)
        {
            return errorResponse("SETEX ttl must be a positive integer");
        }
    case CommandType::Get:
        if (command.arguments.empty())
        {
            return errorResponse("GET requires <key>");
        }

        if (const auto value = storage_->get(command.arguments[0]); value.has_value())
        {
            return valueResponse(*value);
        }
        return valueResponse("(nil)");
    case CommandType::Delete:
        if (command.arguments.empty())
        {
            return errorResponse("DEL requires <key>");
        }

        if (!persistMutation(command))
        {
            return errorResponse("persistence failure");
        }

        return storage_->erase(command.arguments[0])
            ? okResponse("deleted")
            : okResponse("key not found");
    case CommandType::Find:
        if (command.arguments.size() < 2U)
        {
            return errorResponse("FIND requires WHERE value <operator> <number>");
        }

        {
            const auto query_operator = parseQueryOperator(command.arguments[0]);
            if (!query_operator.has_value())
            {
                return errorResponse("unsupported FIND operator");
            }

            try
            {
                size_t processed_characters = 0;
                const long long target_value = stoll(command.arguments[1], &processed_characters);
                if (processed_characters != command.arguments[1].size())
                {
                    return errorResponse("FIND target must be an integer");
                }

                return formatArrayResponse(
                    storage_->findKeysByNumericValue(*query_operator, target_value)
                );
            }
            catch (const exception&)
            {
                return errorResponse("FIND target must be an integer");
            }
        }
    case CommandType::Exists:
        if (command.arguments.empty())
        {
            return errorResponse("EXISTS requires <key>");
        }

        return valueResponse(storage_->exists(command.arguments[0]) ? "true" : "false");
    case CommandType::Unknown:
    default:
        return errorResponse("unknown command. Type HELP for supported commands");
    }
}

bool CacheEngine::persistMutation(const Command& command) const
{
    if (!persistence_)
    {
        return true;
    }

    return persistence_->append(command);
}
} // namespace dce::core
