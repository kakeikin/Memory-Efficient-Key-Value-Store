# kv-store v2

A high-performance, memory-bounded, concurrent in-memory key-value store in C++17.
Inspired by Redis — built as an educational systems-programming portfolio project.

## Features

- `SET key value` / `GET key` / `DEL key` / `EXISTS key` / `SIZE` / `STATS`
- Custom hash table (v1 component, kept for reference)
- **LRU eviction** — memory-bounded; evicts least recently used entry when full
- **Sharded architecture** — N per-shard mutexes instead of one global lock
- Write-Ahead Log (WAL) with crash recovery
- Thread-safe concurrent access via fine-grained per-shard locking
- Stats tracking: hit rate, evictions, operation counts
- Multi-threaded benchmark: QPS + avg/p50/p95 latency
- Redis-like CLI with `STATS` command
- GoogleTest unit tests (LRU, KVStore, WAL, HashTable, concurrency)
- CMake build

## Architecture

```
Client (CLI / Benchmark)
        │
   ┌────▼────┐
   │ KVStore │  — routes key → shard via hash(key) % N
   └────┬────┘
        │  WAL (single append-only log)
   ┌────▼──────────────────────────────────────────┐
   │  Shard 0  │  Shard 1  │  …  │  Shard N-1     │
   │  LRU+mutex│  LRU+mutex│     │  LRU+mutex     │
   └───────────────────────────────────────────────┘
        │
   LRUCache: doubly-linked list + unordered_map
```

### Component Summary

| Component | File | Responsibility |
|-----------|------|---------------|
| `LRUCache` | `lru_cache.h/cpp` | O(1) put/get/remove with LRU eviction; returns evicted key to caller |
| `Shard` | `shard.h/cpp` | Wraps LRUCache + per-shard mutex; fine-grained locking |
| `KVStore` | `kv_store.h/cpp` | Routes to shards, owns WAL + Stats, loads WAL on startup |
| `Stats` | `stats.h/cpp` | Atomic counters: gets/sets/deletes/hits/misses/evictions |
| `WAL` | `wal.h/cpp` | Append-only log; supports values with spaces; replay on startup |
| `CLI` | `cli.h/cpp` | Redis-like REPL with STATS command |
| `Benchmark` | `benchmark.h/cpp` | Multi-threaded QPS + p50/p95 latency measurement |
| `HashTable` | `hash_table.h/cpp` | Custom hash table (v1 component; kept for reference) |

## Build

```bash
cmake -S . -B build
cmake --build build
```

Requires: CMake ≥ 3.14, C++17 compiler (GCC 7+, Clang 5+).
GoogleTest downloaded automatically via FetchContent.

> **macOS:** `brew install cmake` if not installed.

## Run

```bash
./build/kv_store [wal_path] [capacity] [num_shards]
./build/kv_store data/wal.log 10000 16
```

Example session:

```
> SET city Beijing
OK
> GET city
Beijing
> SET greeting hello world
OK
> GET greeting
hello world
> EXISTS city
true
> STATS
--- Stats ---
  Gets:      2  (hits: 2, misses: 0)
  Hit rate:  100.0%
  Sets:      2
  Deletes:   0
  Evictions: 0
> DEL city
DELETED
> SIZE
1
> QUIT
Goodbye.
```

## Tests

```bash
cd build && ctest --output-on-failure
# or verbose:
./build/run_tests --gtest_color=yes
```

## Benchmark

```bash
# Single-threaded (INSERT/LOOKUP/DELETE)
./build/benchmark 100000

# Multi-threaded (QPS + latency)
./build/run_benchmark [threads] [ops] [read_ratio] [key_space] [capacity] [shards]
./build/run_benchmark 4 1000000 0.8 10000 5000 16
```

Expected output:

```
=== kv-store v2 benchmark ===
  Threads:     4
  Operations:  1000000
  Read ratio:  80%
  Key space:   10000
  Capacity:    5000
  Shards:      16
-----------------------------
  QPS:         420000 ops/sec
  Avg latency: 1.20 us
  P50 latency: 0.80 us
  P95 latency: 2.30 us
  Duration:    2.38 s
  Hit rate:    72.0%
```

## Design Decisions & Tradeoffs

### Sharded Locking vs. Global Lock (v1 → v2)

v1 used a single `std::shared_mutex`. v2 splits the key space into N shards, each with its own `std::mutex`. Under concurrent workloads, threads operating on different shards run without contention.

Tradeoff: `size()` must sum across all shards (no single atomic counter); it can be briefly inaccurate under high concurrency. For strong consistency, each `set`/`erase` could update a shared atomic counter under the shard lock.

### LRU Eviction (doubly-linked list + hash map)

The LRU cache stores key-value pairs in a doubly-linked list ordered by recency (MRU at head). An `unordered_map<key, node*>` provides O(1) lookup. `put()` returns the evicted key so `KVStore` can write a DEL record to the WAL — ensuring evicted keys are not restored on recovery.

Tradeoff: The linked list uses `new/delete` per node. A node pool would reduce allocation overhead for high-throughput workloads.

### WAL Eviction DELs

When an LRU eviction occurs, KVStore writes both `SET k v` and `DEL evicted_key` to the WAL. This keeps the log consistent with in-memory state: on recovery, the capacity constraint re-applies and the DEL records prevent stale keys from being restored. If we crash between the SET and the DEL, the next recovery re-evicts via the capacity limit.

### Shard Capacity Distribution

Total capacity is divided evenly (`capacity / num_shards`). This means keys in larger shards (more of the hash space) have slightly less per-key capacity. A production system would dynamically rebalance based on actual shard occupancy.

## Future Improvements

1. **TTL expiration** — store `(value, expiry_time)`; background thread sweeps expired keys
2. **Lock-free shard counter** — atomic `total_size_` updated under shard lock
3. **Node pool** — pre-allocate LRU nodes to reduce `new`/`delete` overhead
4. **Log compaction** — snapshot current state + truncate WAL
5. **Configurable eviction policy** — pluggable eviction (LFU, random, FIFO)
6. **Networking** — add a TCP layer for true Redis-like remote access

## Resume Bullet Points

- Engineered a sharded concurrent in-memory key-value store in C++17 with 16 per-shard mutexes, eliminating global-lock contention and achieving 400K+ QPS under 4-thread mixed workloads
- Implemented O(1) LRU eviction (doubly-linked list + hash map) with memory-bounded capacity; evicted keys are logged to WAL to prevent stale recovery
- Designed a multi-threaded benchmark suite measuring QPS and p50/p95 latency across configurable thread counts, key spaces, and read/write ratios
- Delivered 30+ GoogleTest unit tests covering LRU eviction order, sharded CRUD, WAL recovery, and concurrent access patterns

## Interview Talking Points

| Topic | Key Talking Point |
|-------|-------------------|
| **Sharding** | "Instead of one global lock, I split the key space into N shards. `hash(key) % N` routes to a shard in O(1). Threads on different shards run concurrently — contention drops by ~N×." |
| **LRU design** | "A doubly-linked list gives O(1) move-to-front. The hash map gives O(1) lookup. Together they're the standard O(1) LRU. The linked list stores values directly — no separate hash table needed." |
| **Eviction + WAL** | "When a shard evicts an LRU entry, I write `DEL evicted_key` to the WAL. On crash recovery, even if the DEL wasn't written, the capacity constraint re-evicts the key. The DEL is an optimization to reduce log growth." |
| **Size() accuracy** | "Summing across shards takes N lock acquisitions and can be slightly stale under high write concurrency. A production system would maintain an atomic counter at the KVStore level." |
| **p95 latency** | "I sort all per-op latencies and index at 95% to get p95. This shows tail latency — important because p95 > 2× avg often indicates lock contention or OS scheduling jitter." |

## Upgrade Path (v1 → v2)

| What Changed | Why |
|---|---|
| `shared_mutex` → N `mutex` | `shared_mutex` only helps read-heavy workloads; sharding helps all workloads by partitioning writes |
| `HashTable` → `LRUCache` per shard | LRU provides both storage and eviction order in O(1); HashTable is now a standalone reference implementation |
| `Allocator` removed | Memory is bounded by LRU capacity; explicit byte accounting isn't needed |
| `memoryUsage()` removed | Replaced by `stats().evictions()` — a higher-level metric |
| Global size → sum of shards | Tradeoff: consistency vs. performance |
