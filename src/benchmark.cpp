#include "benchmark.h"
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <random>

namespace kv {

using Clock = std::chrono::high_resolution_clock;
using Ns    = std::chrono::nanoseconds;

BenchmarkResult run_benchmark(const BenchmarkConfig& cfg) {
    std::filesystem::remove(cfg.wal_path);
    KVStore store(cfg.wal_path, cfg.capacity, cfg.num_shards);

    // Pre-populate half the key space so GETs have something to find.
    const size_t prefill = cfg.key_space / 2;
    for (size_t i = 0; i < prefill; ++i) {
        store.set("k" + std::to_string(i), "v" + std::to_string(i));
    }
    // Note: stats() returns const Stats& so reset() cannot be called here.
    // Pre-fill stats will be included but are negligible relative to num_ops.

    const size_t ops_per_thread = cfg.num_ops / cfg.num_threads;

    // Each thread stores its per-op latencies (in nanoseconds).
    std::vector<std::vector<long long>> thread_latencies(cfg.num_threads);
    for (auto& v : thread_latencies) v.reserve(ops_per_thread);

    auto worker = [&](size_t tid) {
        std::mt19937 rng(static_cast<uint32_t>(tid * 1234567u + 42u));
        std::uniform_real_distribution<double> op_dist(0.0, 1.0);
        std::uniform_int_distribution<size_t>  key_dist(0, cfg.key_space - 1);

        auto& lat = thread_latencies[tid];
        for (size_t i = 0; i < ops_per_thread; ++i) {
            std::string key = "k" + std::to_string(key_dist(rng));
            auto t0 = Clock::now();
            if (op_dist(rng) < cfg.read_ratio) {
                store.get(key);
            } else {
                store.set(key, "v");
            }
            auto t1 = Clock::now();
            lat.push_back(std::chrono::duration_cast<Ns>(t1 - t0).count());
        }
    };

    auto wall_start = Clock::now();

    std::vector<std::thread> threads;
    threads.reserve(cfg.num_threads);
    for (size_t t = 0; t < cfg.num_threads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) th.join();

    auto wall_end = Clock::now();
    double duration_sec = std::chrono::duration<double>(wall_end - wall_start).count();

    // Merge all latency samples.
    std::vector<long long> all_latencies;
    all_latencies.reserve(cfg.num_ops);
    for (auto& v : thread_latencies) {
        all_latencies.insert(all_latencies.end(), v.begin(), v.end());
    }
    std::sort(all_latencies.begin(), all_latencies.end());

    const size_t total_ops = all_latencies.size();
    const double avg_ns = static_cast<double>(
        std::accumulate(all_latencies.begin(), all_latencies.end(), 0LL)) / total_ops;
    const double p50_ns = static_cast<double>(all_latencies[total_ops * 50 / 100]);
    const double p95_ns = static_cast<double>(all_latencies[total_ops * 95 / 100]);

    BenchmarkResult r;
    r.qps             = static_cast<double>(total_ops) / duration_sec;
    r.avg_latency_us  = avg_ns / 1000.0;
    r.p50_latency_us  = p50_ns / 1000.0;
    r.p95_latency_us  = p95_ns / 1000.0;
    r.total_ops       = total_ops;
    r.duration_sec    = duration_sec;
    r.hits            = store.stats().hits();
    r.misses          = store.stats().misses();

    std::filesystem::remove(cfg.wal_path);
    return r;
}

void print_result(const BenchmarkConfig& cfg, const BenchmarkResult& r) {
    std::cout << "\n=== kv-store v2 benchmark ===\n"
              << "  Threads:     " << cfg.num_threads << "\n"
              << "  Operations:  " << r.total_ops << "\n"
              << "  Read ratio:  " << static_cast<int>(cfg.read_ratio * 100) << "%\n"
              << "  Key space:   " << cfg.key_space << "\n"
              << "  Capacity:    " << cfg.capacity  << "\n"
              << "  Shards:      " << cfg.num_shards << "\n"
              << "-----------------------------\n"
              << std::fixed << std::setprecision(0)
              << "  QPS:         " << r.qps << " ops/sec\n"
              << std::setprecision(2)
              << "  Avg latency: " << r.avg_latency_us << " us\n"
              << "  P50 latency: " << r.p50_latency_us << " us\n"
              << "  P95 latency: " << r.p95_latency_us << " us\n"
              << "  Duration:    " << r.duration_sec << " s\n"
              << "  Hit rate:    " << std::setprecision(1)
              << (r.hits + r.misses > 0
                  ? 100.0 * r.hits / (r.hits + r.misses) : 0.0) << "%\n";
}

} // namespace kv
