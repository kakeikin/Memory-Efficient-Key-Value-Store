#pragma once

#include <atomic>
#include <cstddef>
#include <string>

namespace kv {

/**
 * Stats — thread-safe operation counters using atomics.
 *
 * Callers do not need to hold any lock to record or read stats.
 * Used by KVStore to track gets/sets/deletes/hits/misses/evictions.
 */
class Stats {
public:
    void recordSet()              { sets_++; }
    void recordGet(bool hit)      { gets_++; if (hit) hits_++; else misses_++; }
    void recordDelete()           { deletes_++; }
    void recordEviction()         { evictions_++; }

    size_t gets()       const { return gets_.load(); }
    size_t sets()       const { return sets_.load(); }
    size_t deletes()    const { return deletes_.load(); }
    size_t hits()       const { return hits_.load(); }
    size_t misses()     const { return misses_.load(); }
    size_t evictions()  const { return evictions_.load(); }

    // Hit rate as a fraction [0.0, 1.0]. Returns 0.0 if no GETs yet.
    double hitRate() const {
        size_t g = gets_.load();
        return g == 0 ? 0.0 : static_cast<double>(hits_.load()) / g;
    }

    void reset() {
        gets_ = sets_ = deletes_ = hits_ = misses_ = evictions_ = 0;
    }

    // Returns a formatted multi-line stats report.
    std::string report() const;

private:
    std::atomic<size_t> gets_{0};
    std::atomic<size_t> sets_{0};
    std::atomic<size_t> deletes_{0};
    std::atomic<size_t> hits_{0};
    std::atomic<size_t> misses_{0};
    std::atomic<size_t> evictions_{0};
};

} // namespace kv
