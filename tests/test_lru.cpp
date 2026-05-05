#include <gtest/gtest.h>
#include "lru_cache.h"

using kv::LRUCache;

TEST(LRUCacheTest, PutAndGet) {
    LRUCache lru(4);
    lru.put("a", "1");
    EXPECT_EQ(lru.get("a"), "1");
}

TEST(LRUCacheTest, GetMissingReturnsNullopt) {
    LRUCache lru(4);
    EXPECT_EQ(lru.get("missing"), std::nullopt);
}

TEST(LRUCacheTest, UpdateExistingKey) {
    LRUCache lru(4);
    lru.put("k", "old");
    lru.put("k", "new");
    EXPECT_EQ(lru.get("k"), "new");
    EXPECT_EQ(lru.size(), 1u);
}

TEST(LRUCacheTest, RemoveKey) {
    LRUCache lru(4);
    lru.put("k", "v");
    EXPECT_TRUE(lru.remove("k"));
    EXPECT_EQ(lru.get("k"), std::nullopt);
    EXPECT_EQ(lru.size(), 0u);
}

TEST(LRUCacheTest, RemoveMissingKeyReturnsFalse) {
    LRUCache lru(4);
    EXPECT_FALSE(lru.remove("missing"));
}

TEST(LRUCacheTest, ContainsKey) {
    LRUCache lru(4);
    lru.put("k", "v");
    EXPECT_TRUE(lru.contains("k"));
    EXPECT_FALSE(lru.contains("other"));
}

TEST(LRUCacheTest, SizeTracking) {
    LRUCache lru(4);
    EXPECT_EQ(lru.size(), 0u);
    lru.put("a", "1");
    lru.put("b", "2");
    EXPECT_EQ(lru.size(), 2u);
    lru.remove("a");
    EXPECT_EQ(lru.size(), 1u);
}

TEST(LRUCacheTest, NoEvictionBelowCapacity) {
    LRUCache lru(3);
    EXPECT_EQ(lru.put("a", "1"), std::nullopt);
    EXPECT_EQ(lru.put("b", "2"), std::nullopt);
    EXPECT_EQ(lru.put("c", "3"), std::nullopt);
    EXPECT_EQ(lru.size(), 3u);
}

TEST(LRUCacheTest, EvictsLRUOnCapacityExceeded) {
    // Insert order: a, b, c (capacity=2).
    // Before 3rd insert: a is LRU -> evicted.
    LRUCache lru(2);
    lru.put("a", "1");
    lru.put("b", "2");
    auto evicted = lru.put("c", "3"); // a is LRU -> evicted
    EXPECT_EQ(evicted, "a");
    EXPECT_EQ(lru.size(), 2u);
    EXPECT_EQ(lru.get("a"), std::nullopt); // evicted
    EXPECT_EQ(lru.get("b"), "2");
    EXPECT_EQ(lru.get("c"), "3");
}

TEST(LRUCacheTest, GetUpdatesLRUOrder) {
    // Insert a, b (capacity=2). Get a (a becomes MRU). Insert c -> b is LRU -> evicted.
    LRUCache lru(2);
    lru.put("a", "1");
    lru.put("b", "2");
    lru.get("a"); // a moves to MRU; b is now LRU
    auto evicted = lru.put("c", "3"); // b is LRU -> evicted
    EXPECT_EQ(evicted, "b");
    EXPECT_EQ(lru.get("a"), "1"); // still present
    EXPECT_EQ(lru.get("c"), "3");
}

TEST(LRUCacheTest, PutUpdateDoesNotEvict) {
    // Updating an existing key should NOT count as a new entry and should NOT evict.
    LRUCache lru(2);
    lru.put("a", "1");
    lru.put("b", "2");
    auto evicted = lru.put("a", "updated"); // update, not new key
    EXPECT_EQ(evicted, std::nullopt);        // no eviction
    EXPECT_EQ(lru.size(), 2u);
    EXPECT_EQ(lru.get("a"), "updated");
}

TEST(LRUCacheTest, ZeroCapacityThrows) {
    EXPECT_THROW(LRUCache(0), std::invalid_argument);
}

TEST(LRUCacheTest, CapacityOneAlwaysEvicts) {
    LRUCache lru(1);
    lru.put("a", "1");
    auto evicted = lru.put("b", "2");
    EXPECT_EQ(evicted, "a");
    EXPECT_EQ(lru.size(), 1u);
    EXPECT_EQ(lru.get("b"), "2");
}

TEST(LRUCacheTest, EvictionOrderIsCorrectAfterMixedOps) {
    // Sequence: put a, put b, put c (cap=2).
    // After put c: a should be evicted (oldest insert, never accessed).
    // Then get b (b->MRU). put d -> c is LRU -> evicted.
    LRUCache lru(2);
    lru.put("a", "1");
    lru.put("b", "2");
    auto ev1 = lru.put("c", "3"); // evicts a
    EXPECT_EQ(ev1, "a");
    lru.get("b");                  // b->MRU, c is LRU
    auto ev2 = lru.put("d", "4"); // evicts c
    EXPECT_EQ(ev2, "c");
    EXPECT_EQ(lru.get("b"), "2");
    EXPECT_EQ(lru.get("d"), "4");
}
