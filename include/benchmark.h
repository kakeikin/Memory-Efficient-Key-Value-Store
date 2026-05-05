#pragma once

#include "kv_store.h"
#include <cstddef>
#include <string>

namespace kv {

struct BenchmarkConfig {
    size_t num_threads  = 4;
    size_t num_ops      = 1'000'000;
    double read_ratio   = 0.8;   // fraction of ops that are GETs
    size_t key_space    = 10000; // number of distinct keys
    size_t capacity     = 5000;  // KVStore total capacity
    size_t num_shards   = 16;
    std::string wal_path = "/tmp/bench_v2_wal.log";
};

struct BenchmarkResult {
    double   qps;              // operations per second
    double   avg_latency_us;   // average latency in microseconds
    double   p50_latency_us;   // median latency
    double   p95_latency_us;   // 95th percentile latency
    size_t   total_ops;
    double   duration_sec;
    size_t   hits;
    size_t   misses;
};

// Runs a multi-threaded benchmark and returns results.
BenchmarkResult run_benchmark(const BenchmarkConfig& config);

// Prints a formatted result to stdout.
void print_result(const BenchmarkConfig& config, const BenchmarkResult& result);

} // namespace kv
