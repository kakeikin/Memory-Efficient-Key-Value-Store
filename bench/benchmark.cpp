#include "kv_store.h"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

using Clock = std::chrono::high_resolution_clock;

static double ops_per_sec(size_t n, std::chrono::nanoseconds elapsed) {
    return static_cast<double>(n) / (static_cast<double>(elapsed.count()) / 1e9);
}

int main(int argc, char* argv[]) {
    const size_t N = (argc > 1) ? static_cast<size_t>(std::stoul(argv[1])) : 100'000;
    const std::string wal_path = "/tmp/bench_wal.log";
    std::filesystem::remove(wal_path);

    // Use large capacity so no eviction interferes with the simple benchmark.
    kv::KVStore store(wal_path, N * 2, 16);

    std::cout << "=== kv-store single-threaded benchmark (" << N << " ops each) ===\n";

    std::vector<std::string> keys(N), vals(N);
    for (size_t i = 0; i < N; ++i) {
        keys[i] = "key" + std::to_string(i);
        vals[i] = "value" + std::to_string(i);
    }

    auto t0 = Clock::now();
    for (size_t i = 0; i < N; ++i) store.set(keys[i], vals[i]);
    auto t1 = Clock::now();
    auto insert_ns = t1 - t0;

    size_t found = 0;
    t0 = Clock::now();
    for (size_t i = 0; i < N; ++i) {
        if (store.get(keys[i]).has_value()) ++found;
    }
    t1 = Clock::now();
    auto lookup_ns = t1 - t0;

    t0 = Clock::now();
    for (size_t i = 0; i < N; ++i) store.erase(keys[i]);
    t1 = Clock::now();
    auto delete_ns = t1 - t0;

    auto ms = [](auto d) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    };
    std::cout
        << "INSERT: " << ms(insert_ns) << " ms  |  "
        << static_cast<size_t>(ops_per_sec(N, insert_ns)) << " ops/sec\n"
        << "LOOKUP: " << ms(lookup_ns) << " ms  |  "
        << static_cast<size_t>(ops_per_sec(N, lookup_ns)) << " ops/sec"
        << "  (found " << found << "/" << N << ")\n"
        << "DELETE: " << ms(delete_ns) << " ms  |  "
        << static_cast<size_t>(ops_per_sec(N, delete_ns)) << " ops/sec\n";

    std::cout << store.stats().report();
    std::filesystem::remove(wal_path);
    return 0;
}
