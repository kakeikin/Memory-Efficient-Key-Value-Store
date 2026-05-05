#pragma once

#include "lru_cache.h"
#include <mutex>
#include <optional>
#include <string>
#include <cstddef>

namespace kv {

/**
 * Shard — one partition of the key space.
 *
 * Each shard owns an LRU cache and an exclusive mutex.
 * Fine-grained locking: only the shard for a given key is locked
 * per operation, allowing concurrent access to different shards.
 *
 * set() returns the evicted key (if any) so KVStore can write DEL to WAL.
 */
class Shard {
public:
    explicit Shard(size_t capacity);

    // Insert/update key. Returns evicted key if LRU eviction occurred.
    std::optional<std::string> set(const std::string& key, const std::string& value);

    // Get value, updating LRU order. Returns nullopt if not found.
    std::optional<std::string> get(const std::string& key);

    // Remove key. Returns true if key existed.
    bool erase(const std::string& key);

    bool exists(const std::string& key) const;
    size_t size() const;

private:
    LRUCache lru_;
    mutable std::mutex mutex_;
};

} // namespace kv
