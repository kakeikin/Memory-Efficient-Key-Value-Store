#include <gtest/gtest.h>
#include "hash_table.h"

using kv::HashTable;

TEST(HashTableTest, InsertNewKeyReturnsTrue) {
    HashTable ht;
    EXPECT_TRUE(ht.insert("key1", "val1"));
}

TEST(HashTableTest, FindAfterInsert) {
    HashTable ht;
    ht.insert("key1", "val1");
    EXPECT_EQ(ht.find("key1"), "val1");
}

TEST(HashTableTest, FindMissingKeyReturnsNullopt) {
    HashTable ht;
    EXPECT_EQ(ht.find("missing"), std::nullopt);
}

TEST(HashTableTest, OverwriteExistingKeyReturnsFalse) {
    HashTable ht;
    ht.insert("key", "old");
    EXPECT_FALSE(ht.insert("key", "new")); // false = updated, not inserted
    EXPECT_EQ(ht.find("key"), "new");
    EXPECT_EQ(ht.size(), 1u);
}

TEST(HashTableTest, EraseExistingKey) {
    HashTable ht;
    ht.insert("key", "val");
    EXPECT_TRUE(ht.erase("key"));
    EXPECT_EQ(ht.find("key"), std::nullopt);
    EXPECT_EQ(ht.size(), 0u);
}

TEST(HashTableTest, EraseMissingKeyReturnsFalse) {
    HashTable ht;
    EXPECT_FALSE(ht.erase("missing"));
}

TEST(HashTableTest, ContainsExistingKey) {
    HashTable ht;
    ht.insert("key", "val");
    EXPECT_TRUE(ht.contains("key"));
    EXPECT_FALSE(ht.contains("other"));
}

TEST(HashTableTest, SizeTracking) {
    HashTable ht;
    EXPECT_EQ(ht.size(), 0u);
    ht.insert("a", "1");
    ht.insert("b", "2");
    EXPECT_EQ(ht.size(), 2u);
    ht.erase("a");
    EXPECT_EQ(ht.size(), 1u);
}

TEST(HashTableTest, LoadFactor) {
    HashTable ht(4);
    EXPECT_DOUBLE_EQ(ht.load_factor(), 0.0);
    ht.insert("a", "1");
    EXPECT_DOUBLE_EQ(ht.load_factor(), 0.25);
}

TEST(HashTableTest, RehashPreservesAllKeys) {
    // capacity=4, load factor threshold=0.75, so rehash triggers at 4th insert
    HashTable ht(4);
    for (int i = 0; i < 20; ++i) {
        ht.insert("key" + std::to_string(i), "val" + std::to_string(i));
    }
    EXPECT_EQ(ht.size(), 20u);
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(ht.find("key" + std::to_string(i)), "val" + std::to_string(i))
            << "key" << i << " missing after rehash";
    }
}

TEST(HashTableTest, RehashDoublesBucketCount) {
    HashTable ht(4);
    EXPECT_EQ(ht.bucket_count(), 4u);
    ht.insert("a", "1");
    ht.insert("b", "2");
    ht.insert("c", "3");
    // Before 4th insert: size_=3, load=3/4=0.75 triggers rehash
    ht.insert("d", "4");
    EXPECT_EQ(ht.bucket_count(), 8u);
    EXPECT_EQ(ht.size(), 4u);
    // All keys still accessible after rehash
    EXPECT_EQ(ht.find("a"), "1");
    EXPECT_EQ(ht.find("d"), "4");
}

TEST(HashTableTest, ZeroCapacityThrows) {
    EXPECT_THROW(HashTable(0), std::invalid_argument);
}

TEST(HashTableTest, CollisionHandling) {
    // With small capacity, force multiple keys to same bucket.
    HashTable ht(2);
    for (int i = 0; i < 10; ++i) {
        ht.insert("k" + std::to_string(i), "v" + std::to_string(i));
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(ht.find("k" + std::to_string(i)), "v" + std::to_string(i));
    }
}
