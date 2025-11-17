#include <avs/core/Pipeline.hpp>

#include <thread>
#include <utility>

namespace avs::core {

Pipeline::Pipeline(EffectRegistry& registry, int numThreads)
    : registry_(registry), threadPool_(nullptr) {
  if (numThreads > 1) {
    threadPool_ = std::make_unique<ThreadPool>(numThreads);
  }
}

void Pipeline::add(std::string key, ParamBlock params) {
  auto effect = registry_.make(key);
  if (!effect) {
    return;
  }
  effect->setParams(params);
  nodes_.push_back(Node{std::move(key), std::move(params), std::move(effect)});
}

bool Pipeline::render(RenderContext& context) {
  context.rng.reseed(context.frameIndex);
  bool success = true;

  for (auto& node : nodes_) {
    if (!node.effect) {
      continue;
    }

    // Check if effect supports multi-threading and pool is available
    if (threadPool_ && threadPool_->isMultiThreaded() && node.effect->supportsMultiThreaded()) {
      // Multi-threaded rendering
      bool renderSuccess = true;
      threadPool_->execute([&](int threadId, int maxThreads) {
        if (!node.effect->smp_render(context, threadId, maxThreads)) {
          renderSuccess = false;
        }
      });

      if (!renderSuccess) {
        success = false;
        break;
      }
    } else {
      // Single-threaded rendering
      if (!node.effect->render(context)) {
        success = false;
        break;
      }
    }
  }

  return success;
}

void Pipeline::clear() { nodes_.clear(); }

void Pipeline::setThreadCount(int numThreads) {
  if (numThreads <= 1) {
    threadPool_.reset();
  } else {
    threadPool_ = std::make_unique<ThreadPool>(numThreads);
  }
}

}  // namespace avs::core
