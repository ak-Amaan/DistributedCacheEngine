#include "storage/store.hpp"

#include <algorithm>
#include <limits>

using namespace std;

namespace dce::storage
{
namespace
{
constexpr auto kCleanupInterval = chrono::milliseconds(500);
}

Store::Store()
    : stop_cleanup_(false), cleanup_thread_(&Store::cleanupLoop, this)
{
}

Store::~Store()
{
    {
        unique_lock<shared_mutex> lock{mutex_};
        stop_cleanup_ = true;
    }

    cleanup_cv_.notify_all();

    if (cleanup_thread_.joinable())
    {
        cleanup_thread_.join();
    }
}

void Store::set(const string& key, const string& value)
{
    unique_lock<shared_mutex> lock{mutex_};
    data_[key] = value;
    expirations_.erase(key);
}

void Store::set(const string& key, const string& value, chrono::seconds ttl)
{
    unique_lock<shared_mutex> lock{mutex_};
    data_[key] = value;
    expirations_[key] = Clock::now() + ttl;
    cleanup_cv_.notify_all();
}

optional<string> Store::get(const string& key) const
{
    {
        shared_lock<shared_mutex> lock{mutex_};
        const auto it = data_.find(key);
        if (it == data_.end())
        {
            return nullopt;
        }

        const auto now = Clock::now();
        if (!isExpiredLocked(key, now))
        {
            return it->second;
        }
    }

    unique_lock<shared_mutex> lock{mutex_};
    const auto it = data_.find(key);
    if (it == data_.end())
    {
        return nullopt;
    }

    const auto now = Clock::now();
    if (isExpiredLocked(key, now))
    {
        data_.erase(it);
        expirations_.erase(key);
        return nullopt;
    }

    return it->second;
}

bool Store::del(const string& key)
{
    unique_lock<shared_mutex> lock{mutex_};
    expirations_.erase(key);
    return data_.erase(key) > 0U;
}

vector<string> Store::findKeysByNumericValue(
    QueryOperator query_operator,
    long long target_value
) const
{
    shared_lock<shared_mutex> lock{mutex_};
    const auto now = Clock::now();

    vector<string> matching_keys;
    for (const auto& [key, value] : data_)
    {
        if (isExpiredLocked(key, now))
        {
            continue;
        }

        try
        {
            size_t processed_characters = 0;
            const long long numeric_value = stoll(value, &processed_characters);
            if (processed_characters != value.size())
            {
                continue;
            }

            if (matchesQuery(numeric_value, query_operator, target_value))
            {
                matching_keys.push_back(key);
            }
        }
        catch (const exception&)
        {
            continue;
        }
    }

    sort(matching_keys.begin(), matching_keys.end());
    return matching_keys;
}

size_t Store::size() const
{
    shared_lock<shared_mutex> lock{mutex_};
    const auto now = Clock::now();
    size_t live_entries = 0;

    for (const auto& [key, _] : data_)
    {
        if (!isExpiredLocked(key, now))
        {
            ++live_entries;
        }
    }

    return live_entries;
}

void Store::cleanupLoop()
{
    unique_lock<shared_mutex> lock{mutex_};

    while (!stop_cleanup_)
    {
        cleanup_cv_.wait_for(lock, kCleanupInterval, [this] { return stop_cleanup_; });
        if (stop_cleanup_)
        {
            break;
        }

        removeExpiredLocked(Clock::now());
    }
}

bool Store::isExpiredLocked(const string& key, TimePoint now) const
{
    const auto expiry_it = expirations_.find(key);
    if (expiry_it == expirations_.end())
    {
        return false;
    }

    return expiry_it->second <= now;
}

void Store::removeExpiredLocked(TimePoint now) const
{
    for (auto it = expirations_.begin(); it != expirations_.end();)
    {
        if (it->second <= now)
        {
            data_.erase(it->first);
            it = expirations_.erase(it);
            continue;
        }

        ++it;
    }
}

bool Store::matchesQuery(
    long long stored_value,
    QueryOperator query_operator,
    long long target_value
)
{
    switch (query_operator)
    {
    case QueryOperator::GreaterThan:
        return stored_value > target_value;
    case QueryOperator::LessThan:
        return stored_value < target_value;
    case QueryOperator::Equal:
        return stored_value == target_value;
    default:
        return false;
    }
}
} // namespace dce::storage
