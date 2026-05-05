#include "kv_store.h"
#include "cli.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const std::string wal_path  = (argc > 1) ? argv[1] : "data/wal.log";
    const size_t capacity       = (argc > 2) ? static_cast<size_t>(std::stoul(argv[2])) : kv::KVStore::kDefaultCapacity;
    const size_t num_shards     = (argc > 3) ? static_cast<size_t>(std::stoul(argv[3])) : kv::KVStore::kDefaultNumShards;

    std::cout << "WAL: " << wal_path
              << "  capacity: " << capacity
              << "  shards: " << num_shards << "\n";

    kv::KVStore store(wal_path, capacity, num_shards);
    kv::CLI cli(store);
    cli.run();
    return 0;
}
