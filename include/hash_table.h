#pragma once
#include <string>
#include <optional>
#include <vector>
#include <list>
#include <cstddef>
namespace kv {
class HashTable {
public:
    explicit HashTable(size_t initial_capacity = 16);
    bool insert(const std::string& key, const std::string& value);
    std::optional<std::string> find(const std::string& key) const;
    bool erase(const std::string& key);
    bool contains(const std::string& key) const;
    size_t size() const;
    size_t bucket_count() const;
    double load_factor() const;
private:
    using Entry = std::pair<std::string, std::string>;
    using Bucket = std::list<Entry>;
    std::vector<Bucket> buckets_;
    size_t size_ = 0;
    static constexpr double kLoadFactorThreshold = 0.75;
    size_t bucket_index(const std::string& key) const;
    void rehash();
};
} // namespace kv
