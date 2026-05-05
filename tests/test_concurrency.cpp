#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <filesystem>
#include <string>
#include "kv_store.h"

// All tests use a fresh WAL file and a store with 4 shards for isolation.
class ConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        wal_path_ = "/tmp/kv_concurrency_" + std::to_string(counter_++) + ".log";
        std::filesystem::remove(wal_path_);
        store_ = std::make_unique<kv::KVStore>(wal_path_, /*capacity=*/10000, /*shards=*/4);
    }
    void TearDown() override {
        store_.reset();
        std::filesystem::remove(wal_path_);
    }
    std::string wal_path_;
    std::unique_ptr<kv::KVStore> store_;
    static int counter_;
};
int ConcurrencyTest::counter_ = 0;

TEST_F(ConcurrencyTest, ConcurrentSetsDifferentKeys) {
    // 8 threads each write 1000 unique keys. No key overlaps -> no contention.
    // After all writes, every key must be retrievable.
    const int THREADS = 8, OPS = 1000;
    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([this, t, OPS]() {
            for (int i = 0; i < OPS; ++i) {
                std::string key = "t" + std::to_string(t) + "k" + std::to_string(i);
                store_->set(key, std::to_string(i));
            }
        });
    }
    for (auto& th : threads) th.join();

    // Spot-check: first and last key from each thread.
    for (int t = 0; t < THREADS; ++t) {
        EXPECT_TRUE(store_->exists("t" + std::to_string(t) + "k0"));
        EXPECT_TRUE(store_->exists("t" + std::to_string(t) + "k" + std::to_string(OPS - 1)));
    }
}

TEST_F(ConcurrencyTest, ConcurrentSetsSameKey) {
    // 8 threads each write to the SAME key 500 times.
    // After all writes, key must exist with some value (no data corruption).
    const int THREADS = 8, OPS = 500;
    std::vector<std::thread> threads;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([this, t, OPS]() {
            for (int i = 0; i < OPS; ++i) {
                store_->set("shared", "t" + std::to_string(t) + "_" + std::to_string(i));
            }
        });
    }
    for (auto& th : threads) th.join();

    // Key must exist (some thread's final write won).
    EXPECT_TRUE(store_->exists("shared"));
    auto val = store_->get("shared");
    EXPECT_TRUE(val.has_value());
}

TEST_F(ConcurrencyTest, ConcurrentReadsWhileWriting) {
    // Pre-populate, then run 4 writer threads and 4 reader threads concurrently.
    // Readers should never crash or return corrupt data.
    for (int i = 0; i < 100; ++i) {
        store_->set("pre" + std::to_string(i), std::to_string(i));
    }

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // Writers: continuously update keys
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, t, &stop]() {
            int i = 0;
            while (!stop.load()) {
                store_->set("w" + std::to_string(t), std::to_string(i++));
            }
        });
    }

    // Readers: continuously read pre-populated keys
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &stop]() {
            int i = 0;
            while (!stop.load()) {
                // Either nullopt (key evicted or deleted) or a string -- both valid.
                (void)store_->get("pre" + std::to_string(i % 100));
                ++i;
            }
        });
    }

    // Let threads run for 200ms then stop.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop.store(true);
    for (auto& th : threads) th.join();

    // No crash = success. Store must still be functional.
    store_->set("final_check", "ok");
    EXPECT_EQ(store_->get("final_check"), "ok");
}

TEST_F(ConcurrencyTest, ConcurrentEraseAndGet) {
    // Insert keys, then concurrently erase half and get all.
    // No undefined behavior allowed -- get may return nullopt if key was erased.
    const int N = 200;
    for (int i = 0; i < N; ++i) {
        store_->set("k" + std::to_string(i), "v" + std::to_string(i));
    }

    std::vector<std::thread> threads;

    // Erase even-numbered keys
    threads.emplace_back([this, N]() {
        for (int i = 0; i < N; i += 2) {
            store_->erase("k" + std::to_string(i));
        }
    });

    // Get all keys (may or may not find them)
    threads.emplace_back([this, N]() {
        for (int i = 0; i < N; ++i) {
            auto v = store_->get("k" + std::to_string(i));
            // v is either nullopt or the original value -- both valid.
            if (v.has_value()) {
                EXPECT_EQ(*v, "v" + std::to_string(i));
            }
        }
    });

    for (auto& th : threads) th.join();

    // Odd keys must still exist (they were never erased).
    for (int i = 1; i < N; i += 2) {
        EXPECT_EQ(store_->get("k" + std::to_string(i)), "v" + std::to_string(i));
    }
}

TEST_F(ConcurrencyTest, ShardingDistributesLoad) {
    // With 4 shards, SIZE is the sum across all shards.
    // Insert keys and verify size is correct.
    const int N = 100;
    for (int i = 0; i < N; ++i) {
        store_->set("key" + std::to_string(i), "val");
    }
    EXPECT_EQ(store_->size(), static_cast<size_t>(N));
}
