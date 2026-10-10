/**
 * Copyright (c) 2026. Released under the BSD license in the project root.
 *
 * Workers publish complete results; the processor thread owns their application.
 */
#pragma once

#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

namespace rex::graphics::vulkan {

template <typename Result>
class PipelineCompletionQueue {
 public:
  void Push(Result result) {
    std::lock_guard lock(mutex_);
    results_.push_back(std::move(result));
    pending_.store(true, std::memory_order_release);
  }

  // No allocation or mutex acquisition occurs on the normal draw path when
  // no compilation has completed.
  std::vector<Result> TakeAll() {
    std::vector<Result> results;
    if (!pending_.load(std::memory_order_acquire)) {
      return results;
    }
    std::lock_guard lock(mutex_);
    results.swap(results_);
    pending_.store(false, std::memory_order_release);
    return results;
  }

 private:
  std::mutex mutex_;
  std::vector<Result> results_;
  std::atomic<bool> pending_{false};
};

}  // namespace rex::graphics::vulkan
