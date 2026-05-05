#include "thread_pool.h"
namespace kv {
ThreadPool::ThreadPool(size_t) {}
ThreadPool::~ThreadPool() {}
void ThreadPool::submit(std::function<void()> task) { task(); } // synchronous
void ThreadPool::wait() {}
} // namespace kv
