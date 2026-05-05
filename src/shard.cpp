#include "shard.h"

namespace kv {

Shard::Shard(size_t capacity) : lru_(capacity) {}

std::optional<std::string> Shard::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_.put(key, value);
}

std::optional<std::string> Shard::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_.get(key);
}

bool Shard::erase(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_.remove(key);
}

bool Shard::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_.contains(key);
}

size_t Shard::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_.size();
}

} // namespace kv
