#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace avs::core {

/**
 * @brief Simple thread pool for parallel effect rendering.
 *
 * This thread pool is designed specifically for the AVS rendering pipeline,
 * where work is distributed across scanlines or pixel rows.
 */
class ThreadPool {
 public:
  /**
   * @brief Construct a thread pool with the specified number of threads.
   *
   * @param numThreads Number of worker threads (0 or 1 = single-threaded)
   */
  explicit ThreadPool(int numThreads);

  /**
   * @brief Destructor - waits for all threads to complete.
   */
  ~ThreadPool();

  /**
   * @brief Execute a task in parallel across all threads.
   *
   * The task function will be called once per thread with the thread ID
   * (0-indexed) and the total number of threads as arguments.
   *
   * This method blocks until all threads have completed the task.
   *
   * @param task Function to execute: (threadId, maxThreads) -> void
   */
  void execute(std::function<void(int, int)> task);

  /**
   * @brief Get the number of threads in this pool.
   *
   * @return Number of worker threads
   */
  int getThreadCount() const { return static_cast<int>(threads_.size()); }

  /**
   * @brief Check if the pool is multi-threaded.
   *
   * @return true if thread count > 1, false otherwise
   */
  bool isMultiThreaded() const { return threads_.size() > 1; }

 private:
  void workerLoop(int threadId);

  std::vector<std::thread> threads_;
  std::function<void(int, int)> currentTask_;
  std::mutex mutex_;
  std::condition_variable taskReady_;
  std::condition_variable taskComplete_;
  std::atomic<bool> shutdown_{false};
  std::atomic<int> activeWorkers_{0};
  std::atomic<int> completedWorkers_{0};
  bool hasTask_{false};
};

}  // namespace avs::core
