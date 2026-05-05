#include "lru_cache.h"

namespace kv {

LRUCache::LRUCache(size_t capacity) : capacity_(capacity) {
    if (capacity == 0) throw std::invalid_argument("LRUCache capacity must be > 0");
    // Sentinel nodes eliminate boundary checks in insert/remove operations.
    head_ = new Node("", "");
    tail_ = new Node("", "");
    head_->next = tail_;
    tail_->prev = head_;
}

LRUCache::~LRUCache() {
    Node* cur = head_;
    while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
}

void LRUCache::detach(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void LRUCache::moveToFront(Node* node) {
    detach(node);
    node->next = head_->next;
    node->prev = head_;
    head_->next->prev = node;
    head_->next = node;
}

LRUCache::Node* LRUCache::evictTail() {
    Node* lru = tail_->prev;
    if (lru == head_) return nullptr; // cache is empty
    detach(lru);
    map_.erase(lru->key);
    return lru;
}

std::optional<std::string> LRUCache::put(const std::string& key, const std::string& value) {
    auto it = map_.find(key);
    if (it != map_.end()) {
        // Key exists — update value and promote to MRU. No eviction.
        it->second->value = value;
        moveToFront(it->second);
        return std::nullopt;
    }

    // New key — insert at front (MRU position).
    Node* node = new Node(key, value);
    node->next = head_->next;
    node->prev = head_;
    head_->next->prev = node;
    head_->next = node;
    map_[key] = node;

    // Evict LRU entry if over capacity.
    if (map_.size() > capacity_) {
        Node* evicted = evictTail();
        if (evicted) {
            std::string evicted_key = evicted->key;
            delete evicted;
            return evicted_key;
        }
    }
    return std::nullopt;
}

std::optional<std::string> LRUCache::get(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    moveToFront(it->second);
    return it->second->value;
}

bool LRUCache::remove(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    detach(it->second);
    delete it->second;
    map_.erase(it);
    return true;
}

bool LRUCache::contains(const std::string& key) const {
    return map_.count(key) > 0;
}

} // namespace kv
