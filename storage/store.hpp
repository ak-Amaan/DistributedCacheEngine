#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "distributed_cache_engine/storage/storage_engine.h"

namespace dce::storage
{
class Store
{
public:
    Store();
    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    Store(Store&&) = delete;
    Store& operator=(Store&&) = delete;

    void set(const std::string& key, const std::string& value);
    void set(
        const std::string& key,
        const std::string& value,
        std::chrono::seconds ttl
    );
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);
    std::vector<std::string> findKeysByNumericValue(
        QueryOperator query_operator,
        long long target_value
    ) const;
    std::size_t size() const;

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void cleanupLoop();
    // We use shared_mutex so read-heavy operations can proceed concurrently.
    // This lets GET/EXISTS/FIND avoid contending on a single exclusive lock
    // while writers and the TTL cleanup thread still take exclusive ownership.
    bool isExpiredLocked(const std::string& key, TimePoint now) const;
    void removeExpiredLocked(TimePoint now) const;
    static bool matchesQuery(
        long long stored_value,
        QueryOperator query_operator,
        long long target_value
    );

    mutable std::shared_mutex mutex_;
    std::condition_variable_any cleanup_cv_;
    bool stop_cleanup_;
    std::thread cleanup_thread_;
    mutable std::unordered_map<std::string, std::string> data_;
    mutable std::unordered_map<std::string, TimePoint> expirations_;
};
} // namespace dce::storage
