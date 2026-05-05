#pragma once
#include <functional>
#include <cstddef>
namespace kv {
/**
 * ThreadPool — Version 1 stub.
 * A real implementation would use a task queue + condition variables to
 * pipeline WAL I/O off the critical path. Intentionally left as a no-op
 * for v1; all tasks run synchronously in the calling thread.
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 0);
    ~ThreadPool();
    void submit(std::function<void()> task);
    void wait();
};
} // namespace kv
