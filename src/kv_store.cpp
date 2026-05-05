#include "kv_store.h"

namespace kv {

KVStore::KVStore(const std::string& wal_path, size_t capacity, size_t num_shards)
    : wal_(wal_path), num_shards_(num_shards) {
    // Distribute capacity evenly across shards; each gets at least 1 slot.
    const size_t shard_capacity = std::max(size_t{1}, capacity / num_shards);
    shards_.reserve(num_shards);
    for (size_t i = 0; i < num_shards; ++i) {
        shards_.push_back(std::make_unique<Shard>(shard_capacity));
    }
    loadFromWAL();
}

size_t KVStore::shardIndex(const std::string& key) const {
    // Same hash function as HashTable for consistency.
    return std::hash<std::string>{}(key) % num_shards_;
}

void KVStore::loadFromWAL() {
    // Replay WAL into shards. We do NOT write back to WAL during replay.
    // Evictions that occur during replay (capacity constraint re-applied)
    // are expected and handled silently.
    wal_.replay(
        [this](const std::string& key, const std::string& value) {
            shards_[shardIndex(key)]->set(key, value);
        },
        [this](const std::string& key) {
            shards_[shardIndex(key)]->erase(key);
        }
    );
}

bool KVStore::set(const std::string& key, const std::string& value) {
    // Write-ahead: log SET before in-memory update.
    if (!wal_.appendSet(key, value)) return false;

    auto evicted = shards_[shardIndex(key)]->set(key, value);

    stats_.recordSet();
    if (evicted) {
        // Log the eviction so that recovery doesn't restore the evicted key.
        wal_.appendDelete(*evicted);
        stats_.recordEviction();
    }
    return true;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    auto result = shards_[shardIndex(key)]->get(key);
    stats_.recordGet(result.has_value());
    return result;
}

bool KVStore::erase(const std::string& key) {
    // Write DEL to WAL before memory update (write-ahead guarantee).
    if (!wal_.appendDelete(key)) return false;
    bool existed = shards_[shardIndex(key)]->erase(key);
    stats_.recordDelete();
    return existed;
}

bool KVStore::exists(const std::string& key) const {
    return shards_[shardIndex(key)]->exists(key);
}

size_t KVStore::size() const {
    size_t total = 0;
    for (const auto& shard : shards_) {
        total += shard->size();
    }
    return total;
}

} // namespace kv
