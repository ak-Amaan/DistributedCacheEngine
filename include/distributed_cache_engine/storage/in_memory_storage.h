#pragma once

#include "distributed_cache_engine/storage/storage_engine.h"
#include "storage/store.hpp"

#include <chrono>

namespace dce::storage
{
class InMemoryStorage final : public StorageEngine
{
public:
    void set(const std::string& key, const std::string& value) override;
    void set(
        const std::string& key,
        const std::string& value,
        std::chrono::seconds ttl
    ) override;
    std::optional<std::string> get(const std::string& key) const override;
    bool erase(const std::string& key) override;
    bool exists(const std::string& key) const override;
    std::vector<std::string> findKeysByNumericValue(
        QueryOperator query_operator,
        long long target_value
    ) const override;
    std::size_t size() const override;

private:
    Store store_;
};
} // namespace dce::storage
