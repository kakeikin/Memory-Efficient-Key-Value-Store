#pragma once
#include <cstddef>
#include <atomic>
namespace kv {
class Allocator {
public:
    void recordAlloc(size_t bytes);
    void recordFree(size_t bytes);
    size_t bytesInUse() const;
    size_t totalAllocated() const;
private:
    std::atomic<size_t> bytes_in_use_{0};
    std::atomic<size_t> total_allocated_{0};
};
} // namespace kv
