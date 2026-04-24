#include "distributed_cache_engine/storage/in_memory_storage.h"

using namespace std;

namespace dce::storage
{
void InMemoryStorage::set(const std::string& key, const std::string& value)
{
    store_.set(key, value);
}

void InMemoryStorage::set(
    const std::string& key,
    const std::string& value,
    chrono::seconds ttl
)
{
    store_.set(key, value, ttl);
}

std::optional<std::string> InMemoryStorage::get(const std::string& key) const
{
    return store_.get(key);
}

bool InMemoryStorage::erase(const std::string& key)
{
    return store_.del(key);
}

bool InMemoryStorage::exists(const std::string& key) const
{
    return store_.get(key).has_value();
}

vector<string> InMemoryStorage::findKeysByNumericValue(
    QueryOperator query_operator,
    long long target_value
) const
{
    return store_.findKeysByNumericValue(query_operator, target_value);
}

std::size_t InMemoryStorage::size() const
{
    return store_.size();
}
} // namespace dce::storage
