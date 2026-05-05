#include "hash_table.h"
#include <stdexcept>
#include <functional>

namespace kv {

static size_t validated_capacity(size_t cap) {
    if (cap == 0) throw std::invalid_argument("HashTable capacity must be > 0");
    return cap;
}

HashTable::HashTable(size_t initial_capacity)
    : buckets_(validated_capacity(initial_capacity)), size_(0) {}

size_t HashTable::bucket_index(const std::string& key) const {
    // std::hash gives a good distribution; mod by bucket count maps to slot.
    return std::hash<std::string>{}(key) % buckets_.size();
}

bool HashTable::insert(const std::string& key, const std::string& value) {
    // Rehash BEFORE inserting so the check uses the pre-insert count.
    if (load_factor() >= kLoadFactorThreshold) {
        rehash();
    }

    auto& bucket = buckets_[bucket_index(key)];
    for (auto& entry : bucket) {
        if (entry.first == key) {
            entry.second = value; // overwrite existing value
            return false;         // false = updated (not a new key)
        }
    }
    bucket.emplace_back(key, value);
    ++size_;
    return true; // true = new key inserted
}

std::optional<std::string> HashTable::find(const std::string& key) const {
    const auto& bucket = buckets_[bucket_index(key)];
    for (const auto& entry : bucket) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    return std::nullopt;
}

bool HashTable::erase(const std::string& key) {
    auto& bucket = buckets_[bucket_index(key)];
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->first == key) {
            bucket.erase(it);
            --size_;
            return true;
        }
    }
    return false;
}

bool HashTable::contains(const std::string& key) const {
    return find(key).has_value();
}

size_t HashTable::size() const { return size_; }
size_t HashTable::bucket_count() const { return buckets_.size(); }

double HashTable::load_factor() const {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

void HashTable::rehash() {
    // Double the bucket count and redistribute all existing entries.
    // Cost: O(n) — amortized O(1) per insert because rehash frequency halves
    // each time (similar to std::vector growth).
    const size_t new_capacity = buckets_.size() * 2;
    std::vector<Bucket> new_buckets(new_capacity);

    for (auto& bucket : buckets_) {
        for (auto& entry : bucket) {
            size_t idx = std::hash<std::string>{}(entry.first) % new_capacity;
            new_buckets[idx].emplace_back(std::move(entry));
        }
    }

    buckets_ = std::move(new_buckets);
    // size_ is unchanged — we moved entries, didn't add or remove them.
}

} // namespace kv
