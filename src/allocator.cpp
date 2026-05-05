#include "allocator.h"
namespace kv {
void Allocator::recordAlloc(size_t b) { bytes_in_use_ += b; total_allocated_ += b; }
void Allocator::recordFree(size_t b) {
    // NOTE: load() + conditional store/subtract is not a single atomic operation.
    // This is safe because all callers (KVStore::set, KVStore::erase, loadFromWAL)
    // hold the KVStore's exclusive mutex_ before calling here, serializing all
    // recordFree invocations. Do NOT call this without the exclusive lock.
    if (b > bytes_in_use_.load()) {
        bytes_in_use_ = 0;
    } else {
        bytes_in_use_ -= b;
    }
}
size_t Allocator::bytesInUse() const { return bytes_in_use_.load(); }
size_t Allocator::totalAllocated() const { return total_allocated_.load(); }
} // namespace kv
