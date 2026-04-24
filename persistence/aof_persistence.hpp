#pragma once

#include "distributed_cache_engine/core/command.h"
#include "distributed_cache_engine/parser/command_parser.h"
#include "distributed_cache_engine/storage/storage_engine.h"

#include <fstream>
#include <mutex>
#include <string>

namespace dce::persistence
{
class AofPersistence
{
public:
    explicit AofPersistence(std::string log_path);

    bool append(const core::Command& command);
    bool replay(
        storage::StorageEngine& storage,
        const parser::CommandParser& parser
    ) const;

private:
    bool ensureStreamOpen();
    static std::string serialize(const core::Command& command);

    std::string log_path_;
    mutable std::mutex mutex_;
    mutable std::ofstream stream_;
};
} // namespace dce::persistence
