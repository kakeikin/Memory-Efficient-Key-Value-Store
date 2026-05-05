#include "benchmark.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    kv::BenchmarkConfig cfg;

    // Simple positional arguments: threads ops read_ratio key_space capacity shards
    if (argc > 1) cfg.num_threads  = static_cast<size_t>(std::stoul(argv[1]));
    if (argc > 2) cfg.num_ops      = static_cast<size_t>(std::stoul(argv[2]));
    if (argc > 3) cfg.read_ratio   = std::stod(argv[3]);
    if (argc > 4) cfg.key_space    = static_cast<size_t>(std::stoul(argv[4]));
    if (argc > 5) cfg.capacity     = static_cast<size_t>(std::stoul(argv[5]));
    if (argc > 6) cfg.num_shards   = static_cast<size_t>(std::stoul(argv[6]));

    auto result = kv::run_benchmark(cfg);
    kv::print_result(cfg, result);
    return 0;
}
