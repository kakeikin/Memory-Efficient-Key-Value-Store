#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <cstddef>
#include <stdexcept>

namespace kv {

/**
 * LRU Cache — doubly-linked list + hash map for O(1) get/put/remove.
 *
 * Most recently used at head, least recently used at tail.
 * put() returns the evicted key if capacity was exceeded, so the caller
 * can append a DEL record to the WAL.
 *
 * Contains sentinel head/tail nodes to avoid boundary-condition branches.
 */
class LRUCache {
public:
    struct Node {
        std::string key;
        std::string value;
        Node* prev = nullptr;
        Node* next = nullptr;
        Node(std::string k, std::string v)
            : key(std::move(k)), value(std::move(v)) {}
    };

    explicit LRUCache(size_t capacity);
    ~LRUCache();

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // Insert or update key. Returns evicted key if capacity exceeded.
    std::optional<std::string> put(const std::string& key, const std::string& value);

    // Get value and move key to MRU position. Returns nullopt if not found.
    std::optional<std::string> get(const std::string& key);

    // Remove key. Returns true if key existed.
    bool remove(const std::string& key);

    // Check existence without affecting LRU order.
    bool contains(const std::string& key) const;

    size_t size() const { return map_.size(); }
    size_t capacity() const { return capacity_; }

private:
    void moveToFront(Node* node);
    void detach(Node* node);
    // Removes and returns the LRU node pointer (caller must delete).
    Node* evictTail();

    size_t capacity_;
    Node* head_; // sentinel — MRU side
    Node* tail_; // sentinel — LRU side
    std::unordered_map<std::string, Node*> map_;
};

} // namespace kv
