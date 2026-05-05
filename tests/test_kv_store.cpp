#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <string>
#include "kv_store.h"

class KVStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        wal_path_ = "/tmp/kv_test_store_v2_" + std::to_string(counter_++) + ".log";
        std::filesystem::remove(wal_path_);
        // Small shards (2 shards, capacity 100) to keep tests fast.
        store_ = std::make_unique<kv::KVStore>(wal_path_, /*capacity=*/100, /*shards=*/2);
    }
    void TearDown() override {
        store_.reset();
        std::filesystem::remove(wal_path_);
    }
    std::string wal_path_;
    std::unique_ptr<kv::KVStore> store_;
    static int counter_;
};
int KVStoreTest::counter_ = 0;

// ── Basic CRUD ────────────────────────────────────────────────────────────────

TEST_F(KVStoreTest, SetAndGet) {
    store_->set("name", "Jiaxin");
    EXPECT_EQ(store_->get("name"), "Jiaxin");
}

TEST_F(KVStoreTest, GetMissingKeyReturnsNullopt) {
    EXPECT_EQ(store_->get("missing"), std::nullopt);
}

TEST_F(KVStoreTest, OverwriteKey) {
    store_->set("key", "first");
    store_->set("key", "second");
    EXPECT_EQ(store_->get("key"), "second");
    EXPECT_EQ(store_->size(), 1u);
}

TEST_F(KVStoreTest, DeleteExistingKey) {
    store_->set("key", "val");
    EXPECT_TRUE(store_->erase("key"));
    EXPECT_EQ(store_->get("key"), std::nullopt);
}

TEST_F(KVStoreTest, DeleteMissingKeyReturnsFalse) {
    EXPECT_FALSE(store_->erase("missing"));
}

TEST_F(KVStoreTest, ExistsKey) {
    store_->set("key", "val");
    EXPECT_TRUE(store_->exists("key"));
    EXPECT_FALSE(store_->exists("other"));
}

TEST_F(KVStoreTest, SizeTracking) {
    EXPECT_EQ(store_->size(), 0u);
    store_->set("a", "1");
    store_->set("b", "2");
    EXPECT_EQ(store_->size(), 2u);
    store_->erase("a");
    EXPECT_EQ(store_->size(), 1u);
}

// ── LRU Eviction ─────────────────────────────────────────────────────────────

TEST_F(KVStoreTest, EvictsWhenCapacityExceeded) {
    // Use a tiny store: 2 shards, capacity=4 (2 per shard).
    // Insert 20 keys and verify size never exceeds total capacity.
    kv::KVStore tiny("/tmp/kv_tiny_evict_test.log", /*capacity=*/4, /*shards=*/2);

    for (int i = 0; i < 20; ++i) {
        tiny.set("key" + std::to_string(i), "val" + std::to_string(i));
    }
    // Size must not exceed total capacity (4).
    EXPECT_LE(tiny.size(), 4u);
    std::filesystem::remove("/tmp/kv_tiny_evict_test.log");
}

TEST_F(KVStoreTest, StatsTracksHitsAndMisses) {
    store_->set("k", "v");
    store_->get("k");       // hit
    store_->get("missing"); // miss

    EXPECT_EQ(store_->stats().hits(),   1u);
    EXPECT_EQ(store_->stats().misses(), 1u);
    EXPECT_EQ(store_->stats().sets(),   1u);
}

TEST_F(KVStoreTest, StatsTracksEvictions) {
    // capacity=2, shards=1 => 2 per shard. Insert 3 keys => 1 eviction.
    kv::KVStore small("/tmp/kv_eviction_stats_test.log", /*capacity=*/2, /*shards=*/1);
    small.set("a", "1");
    small.set("b", "2");
    small.set("c", "3"); // evicts a (LRU)

    EXPECT_GE(small.stats().evictions(), 1u);
    std::filesystem::remove("/tmp/kv_eviction_stats_test.log");
}

// ── WAL Recovery ─────────────────────────────────────────────────────────────

TEST_F(KVStoreTest, RecoveryRestoresState) {
    store_->set("x", "100");
    store_->set("y", "200");
    store_->erase("x");
    store_.reset(); // flush WAL

    kv::KVStore recovered(wal_path_, 100, 2);
    EXPECT_EQ(recovered.get("x"), std::nullopt);
    EXPECT_EQ(recovered.get("y"), "200");
    EXPECT_EQ(recovered.size(), 1u);
}

TEST_F(KVStoreTest, RecoveryOverwrittenKey) {
    store_->set("k", "v1");
    store_->set("k", "v2");
    store_.reset();

    kv::KVStore recovered(wal_path_, 100, 2);
    EXPECT_EQ(recovered.get("k"), "v2");
}

// ── Thread Safety ─────────────────────────────────────────────────────────────

TEST_F(KVStoreTest, ConcurrentReadsDoNotRace) {
    store_->set("shared", "value");

    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([this]() {
            for (int j = 0; j < 200; ++j) {
                EXPECT_EQ(store_->get("shared"), "value");
            }
        });
    }
    for (auto& t : readers) t.join();
}

TEST_F(KVStoreTest, ConcurrentWritesAndReadsDoNotRace) {
    const int N = 50;
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, N]() {
            for (int j = 0; j < N; ++j) {
                store_->set("key" + std::to_string(i), std::to_string(j));
            }
        });
    }
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, N]() {
            for (int j = 0; j < N; ++j) {
                store_->get("key0");
                store_->size();
            }
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(store_->exists("key" + std::to_string(i)));
    }
}
