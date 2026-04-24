#include "persistence/aof_persistence.hpp"

#include "distributed_cache_engine/utils/logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>

using namespace std;

namespace dce::persistence
{
AofPersistence::AofPersistence(string log_path)
    : log_path_(std::move(log_path))
{
}

bool AofPersistence::append(const core::Command& command)
{
    const string record = serialize(command);
    if (record.empty())
    {
        return true;
    }

    lock_guard<mutex> lock{mutex_};
    if (!ensureStreamOpen())
    {
        return false;
    }

    stream_ << record << '\n';
    stream_.flush();
    return stream_.good();
}

bool AofPersistence::replay(
    storage::StorageEngine& storage,
    const parser::CommandParser& parser
) const
{
    lock_guard<mutex> lock{mutex_};

    const filesystem::path path{log_path_};
    if (!filesystem::exists(path))
    {
        return true;
    }

    ifstream input{path};
    if (!input.is_open())
    {
        return false;
    }

    string line;
    size_t line_number = 0;
    while (getline(input, line))
    {
        ++line_number;
        if (line.empty())
        {
            continue;
        }

        const auto command = parser.parse(line);
        if (!command.has_value())
        {
            dce::utils::Logger::instance().error(
                "Skipping malformed AOF entry at line " + to_string(line_number)
            );
            continue;
        }

        switch (command->type)
        {
        case core::CommandType::Set:
            if (command->arguments.size() >= 2U)
            {
                storage.set(command->arguments[0], command->arguments[1]);
            }
            break;
        case core::CommandType::SetEx:
            if (command->arguments.size() >= 3U)
            {
                try
                {
                    const long long ttl_value = stoll(command->arguments[1]);
                    if (ttl_value > 0 && ttl_value <= numeric_limits<int>::max())
                    {
                        storage.set(
                            command->arguments[0],
                            command->arguments[2],
                            chrono::seconds(ttl_value)
                        );
                    }
                }
                catch (const exception&)
                {
                    dce::utils::Logger::instance().error(
                        "Skipping invalid SETEX AOF entry at line " + to_string(line_number)
                    );
                }
            }
            break;
        case core::CommandType::Delete:
            if (!command->arguments.empty())
            {
                storage.erase(command->arguments[0]);
            }
            break;
        default:
            break;
        }
    }

    return input.good() || input.eof();
}

bool AofPersistence::ensureStreamOpen()
{
    if (stream_.is_open())
    {
        return true;
    }

    const filesystem::path path{log_path_};
    if (path.has_parent_path())
    {
        error_code error;
        filesystem::create_directories(path.parent_path(), error);
        if (error)
        {
            return false;
        }
    }

    stream_.open(log_path_, ios::out | ios::app);
    return stream_.is_open();
}

string AofPersistence::serialize(const core::Command& command)
{
    switch (command.type)
    {
    case core::CommandType::Set:
    case core::CommandType::SetEx:
    case core::CommandType::Delete:
        return command.raw;
    default:
        return {};
    }
}
} // namespace dce::persistence
