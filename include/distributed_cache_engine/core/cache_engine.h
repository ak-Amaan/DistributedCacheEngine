#pragma once

#include "distributed_cache_engine/core/command.h"
#include "distributed_cache_engine/storage/storage_engine.h"

#include <memory>
#include <string>

namespace dce::persistence
{
class AofPersistence;
}

namespace dce::core
{
class CacheEngine
{
public:
    CacheEngine(
        std::shared_ptr<storage::StorageEngine> storage,
        std::shared_ptr<persistence::AofPersistence> persistence = nullptr
    );

    std::string execute(const Command& command);

private:
    bool persistMutation(const Command& command) const;

    std::shared_ptr<storage::StorageEngine> storage_;
    std::shared_ptr<persistence::AofPersistence> persistence_;
};
} // namespace dce::core
