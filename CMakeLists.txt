cmake_minimum_required(VERSION 3.14)
project(kv_store VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic)

# ── Core library ──────────────────────────────────────────────────────────────
add_library(kv_store_lib
    src/hash_table.cpp
    src/wal.cpp
    src/allocator.cpp
    src/thread_pool.cpp
    src/lru_cache.cpp
    src/shard.cpp
    src/stats.cpp
    src/kv_store.cpp
    src/cli.cpp
    src/benchmark.cpp
)
target_include_directories(kv_store_lib PUBLIC include)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(kv_store main.cpp)
target_link_libraries(kv_store PRIVATE kv_store_lib)

# ── GoogleTest ────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(run_tests
    tests/test_hash_table.cpp
    tests/test_wal.cpp
    tests/test_kv_store.cpp
    tests/test_lru.cpp
    tests/test_concurrency.cpp
)
target_link_libraries(run_tests PRIVATE kv_store_lib GTest::gtest_main)
target_include_directories(run_tests PRIVATE include)

include(GoogleTest)
gtest_discover_tests(run_tests)

# ── Benchmarks ────────────────────────────────────────────────────────────────
# Legacy single-threaded benchmark
add_executable(benchmark bench/benchmark.cpp)
target_link_libraries(benchmark PRIVATE kv_store_lib)

# New multi-threaded benchmark with QPS + latency
add_executable(run_benchmark bench/run_benchmark.cpp)
target_link_libraries(run_benchmark PRIVATE kv_store_lib)
