#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace dce::storage
{
enum class QueryOperator
{
    GreaterThan,
    LessThan,
    Equal
};

class StorageEngine
{
public:
    virtual ~StorageEngine() = default;

    virtual void set(const std::string& key, const std::string& value) = 0;
    virtual void set(
        const std::string& key,
        const std::string& value,
        std::chrono::seconds ttl
    ) = 0;
    virtual std::optional<std::string> get(const std::string& key) const = 0;
    virtual bool erase(const std::string& key) = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual std::vector<std::string> findKeysByNumericValue(
        QueryOperator query_operator,
        long long target_value
    ) const = 0;
    virtual std::size_t size() const = 0;
};
} // namespace dce::storage
