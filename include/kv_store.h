#pragma once

#include "shard.h"
#include "wal.h"
#include "stats.h"
#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <cstddef>
#include <functional>

namespace kv {

/**
 * KVStore — sharded, memory-bounded, concurrent key-value store.
 *
 * The key space is partitioned into N shards (default 16). Each shard
 * has its own LRU cache and mutex, eliminating the global lock bottleneck.
 *
 * Memory is bounded: when a shard is full, the LRU entry is evicted and
 * a DEL record is written to WAL so recovery doesn't restore evicted keys.
 *
 * NOTE: Keys must not contain whitespace. Values must not contain newlines.
 *       These constraints come from the text-based WAL format.
 */
class KVStore {
public:
    static constexpr size_t kDefaultNumShards = 16;
    static constexpr size_t kDefaultCapacity  = 10000;

    explicit KVStore(const std::string& wal_path = "data/wal.log",
                     size_t capacity   = kDefaultCapacity,
                     size_t num_shards = kDefaultNumShards);

    // Returns true on success, false if WAL write fails.
    bool set(const std::string& key, const std::string& value);

    std::optional<std::string> get(const std::string& key);
    bool erase(const std::string& key);
    bool exists(const std::string& key) const;
    size_t size() const;

    const Stats& stats() const { return stats_; }

private:
    size_t shardIndex(const std::string& key) const;
    void loadFromWAL();

    std::vector<std::unique_ptr<Shard>> shards_;
    WAL wal_;
    Stats stats_;
    size_t num_shards_;
};

} // namespace kv
