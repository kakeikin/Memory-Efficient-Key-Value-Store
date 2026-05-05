#include <gtest/gtest.h>
#include <filesystem>
#include <map>
#include "wal.h"

// Isolated temp file per test to avoid cross-test state.
class WALTest : public ::testing::Test {
protected:
    void SetUp() override {
        wal_path_ = "/tmp/kv_test_wal_" + std::to_string(counter_++) + ".log";
        std::filesystem::remove(wal_path_); // ensure clean slate
    }
    void TearDown() override {
        std::filesystem::remove(wal_path_);
    }
    std::string wal_path_;
    static int counter_;
};
int WALTest::counter_ = 0;

TEST_F(WALTest, AppendSetWritesRecord) {
    {
        kv::WAL wal(wal_path_);
        EXPECT_TRUE(wal.appendSet("key1", "val1"));
        EXPECT_TRUE(wal.appendSet("key2", "val2"));
    } // destructor flushes + closes

    // File must exist and be non-empty.
    EXPECT_TRUE(std::filesystem::exists(wal_path_));
    EXPECT_GT(std::filesystem::file_size(wal_path_), 0u);
}

TEST_F(WALTest, ReplaySetRecords) {
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("key1", "val1");
        wal.appendSet("key2", "val2");
    }

    std::map<std::string, std::string> recovered;
    kv::WAL wal(wal_path_);
    size_t count = wal.replay(
        [&](const std::string& k, const std::string& v) { recovered[k] = v; },
        [&](const std::string& k) { recovered.erase(k); }
    );

    EXPECT_EQ(count, 2u);
    EXPECT_EQ(recovered["key1"], "val1");
    EXPECT_EQ(recovered["key2"], "val2");
}

TEST_F(WALTest, ReplayDeleteRecord) {
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("key", "val");
        wal.appendDelete("key");
    }

    std::map<std::string, std::string> recovered;
    kv::WAL wal(wal_path_);
    size_t count = wal.replay(
        [&](const std::string& k, const std::string& v) { recovered[k] = v; },
        [&](const std::string& k) { recovered.erase(k); }
    );

    EXPECT_EQ(count, 2u);
    EXPECT_TRUE(recovered.empty()); // SET then DELETE = gone
}

TEST_F(WALTest, ReplayOnEmptyFileReturnsZero) {
    // Create an empty WAL file and replay — should return 0 records.
    kv::WAL wal(wal_path_);
    size_t count = wal.replay(
        [](const std::string&, const std::string&) {},
        [](const std::string&) {}
    );
    EXPECT_EQ(count, 0u);
}

TEST_F(WALTest, ReplayOrderPreserved) {
    // Second SET overwrites first — final value must be "second".
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("key", "first");
        wal.appendSet("key", "second");
    }

    std::string last_value;
    kv::WAL wal(wal_path_);
    wal.replay(
        [&](const std::string&, const std::string& v) { last_value = v; },
        [](const std::string&) {}
    );
    EXPECT_EQ(last_value, "second");
}

TEST_F(WALTest, ValueWithSpacesPreserved) {
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("greeting", "hello world");
        wal.appendSet("sentence", "the quick brown fox");
    }

    std::map<std::string, std::string> recovered;
    kv::WAL wal(wal_path_);
    size_t count = wal.replay(
        [&](const std::string& k, const std::string& v) { recovered[k] = v; },
        [](const std::string&) {}
    );

    EXPECT_EQ(count, 2u);
    EXPECT_EQ(recovered["greeting"], "hello world");
    EXPECT_EQ(recovered["sentence"], "the quick brown fox");
}

TEST_F(WALTest, AppendAfterReopen) {
    // Reopen should append, not truncate.
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("a", "1");
    }
    {
        kv::WAL wal(wal_path_);
        wal.appendSet("b", "2");
    }

    std::map<std::string, std::string> recovered;
    kv::WAL wal(wal_path_);
    wal.replay(
        [&](const std::string& k, const std::string& v) { recovered[k] = v; },
        [](const std::string&) {}
    );

    EXPECT_EQ(recovered["a"], "1");
    EXPECT_EQ(recovered["b"], "2");
}
