#include <avs/core/ThreadPool.hpp>

#include <algorithm>

namespace avs::core {

ThreadPool::ThreadPool(int numThreads) {
  if (numThreads <= 1) {
    // Single-threaded mode - no worker threads needed
    return;
  }

  // Create worker threads
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i) {
    threads_.emplace_back(&ThreadPool::workerLoop, this, i);
  }
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    hasTask_ = true;  // Wake up threads
  }
  taskReady_.notify_all();

  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void ThreadPool::execute(std::function<void(int, int)> task) {
  if (threads_.empty()) {
    // Single-threaded execution
    task(0, 1);
    return;
  }

  // Multi-threaded execution
  {
    std::lock_guard<std::mutex> lock(mutex_);
    currentTask_ = std::move(task);
    hasTask_ = true;
    completedWorkers_ = 0;
  }

  // Wake up all worker threads
  taskReady_.notify_all();

  // Wait for all workers to complete
  {
    std::unique_lock<std::mutex> lock(mutex_);
    taskComplete_.wait(lock, [this] {
      return completedWorkers_ == static_cast<int>(threads_.size());
    });
    hasTask_ = false;
  }
}

void ThreadPool::workerLoop(int threadId) {
  while (true) {
    std::function<void(int, int)> task;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      taskReady_.wait(lock, [this] { return hasTask_; });

      if (shutdown_) {
        return;
      }

      task = currentTask_;
    }

    // Execute the task
    if (task) {
      task(threadId, static_cast<int>(threads_.size()));
    }

    // Signal completion
    {
      std::lock_guard<std::mutex> lock(mutex_);
      completedWorkers_++;
      if (completedWorkers_ == static_cast<int>(threads_.size())) {
        taskComplete_.notify_one();
      }
    }
  }
}

}  // namespace avs::core
