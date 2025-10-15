#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "avs/core/EffectRegistry.hpp"
#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"

namespace {

using avs::core::ParamBlock;
using avs::core::RenderContext;

class LoggingEffect : public avs::core::IEffect {
 public:
  explicit LoggingEffect(std::vector<std::string>* log) : log_(log) {}

  bool render(RenderContext&) override {
    if (log_) {
      log_->push_back(name_);
    }
    return true;
  }

  void setParams(const ParamBlock& params) override {
    name_ = params.getString("name", name_);
  }

 private:
  std::vector<std::string>* log_;
  std::string name_{"unnamed"};
};

class IncrementEffect : public avs::core::IEffect {
 public:
  bool render(RenderContext& ctx) override {
    if (ctx.framebuffer.data && ctx.framebuffer.size > 0) {
      ctx.framebuffer.data[0] = static_cast<std::uint8_t>(
          std::min<int>(255, ctx.framebuffer.data[0] + increment_));
    }
    return true;
  }

  void setParams(const ParamBlock& params) override {
    increment_ = params.getInt("increment", increment_);
  }

 private:
  int increment_{0};
};

class NoiseEffect : public avs::core::IEffect {
 public:
  bool render(RenderContext& ctx) override {
    if (!ctx.framebuffer.data) {
      return true;
    }
    ctx.rng.reseed(ctx.frameIndex);
    for (std::size_t i = 0; i < ctx.framebuffer.size; ++i) {
      ctx.framebuffer.data[i] = static_cast<std::uint8_t>(ctx.rng.nextUint32() & 0xFFu);
    }
    return true;
  }

  void setParams(const ParamBlock&) override {}
};

class NoOpEffect : public avs::core::IEffect {
 public:
  bool render(RenderContext&) override { return true; }
  void setParams(const ParamBlock&) override {}
};

class FlagEffect : public avs::core::IEffect {
 public:
  FlagEffect(bool shouldSucceed, bool* executed)
      : shouldSucceed_(shouldSucceed), executed_(executed) {}

  bool render(RenderContext&) override {
    if (executed_) {
      *executed_ = true;
    }
    return shouldSucceed_;
  }

  void setParams(const ParamBlock&) override {}

 private:
  bool shouldSucceed_;
  bool* executed_;
};

std::string hashFNV1a(const std::vector<std::uint8_t>& data) {
  constexpr std::uint64_t kOffset = 1469598103934665603ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffset;
  for (auto byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

RenderContext makeContext(std::vector<std::uint8_t>& pixels) {
  RenderContext ctx;
  ctx.frameIndex = 3;
  ctx.deltaSeconds = 1.0 / 60.0;
  ctx.width = 2;
  ctx.height = 2;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioSpectrum = {nullptr, 0};
  return ctx;
}

}  // namespace

TEST(PipelineTest, CallsEffectsInOrder) {
  avs::core::EffectRegistry registry;
  std::vector<std::string> callLog;
  registry.registerFactory("log", [&callLog]() {
    return std::make_unique<LoggingEffect>(&callLog);
  });

  avs::core::Pipeline pipeline(registry);
  ParamBlock firstParams;
  firstParams.setString("name", "first");
  pipeline.add("log", firstParams);
  ParamBlock secondParams;
  secondParams.setString("name", "second");
  pipeline.add("log", secondParams);

  std::vector<std::uint8_t> pixels(4u * 4u, 0);
  auto ctx = makeContext(pixels);
  pipeline.render(ctx);

  ASSERT_EQ(callLog.size(), 2u);
  EXPECT_EQ(callLog[0], "first");
  EXPECT_EQ(callLog[1], "second");
}

TEST(PipelineTest, AccumulatesEffectResults) {
  avs::core::EffectRegistry registry;
  registry.registerFactory("inc", []() { return std::make_unique<IncrementEffect>(); });
  registry.registerFactory("noop", []() { return std::make_unique<NoOpEffect>(); });

  avs::core::Pipeline pipeline(registry);
  ParamBlock paramsA;
  paramsA.setInt("increment", 10);
  pipeline.add("inc", paramsA);
  ParamBlock paramsB;
  paramsB.setInt("increment", 5);
  pipeline.add("inc", paramsB);
  pipeline.add("noop", ParamBlock{});

  std::vector<std::uint8_t> pixels(16, 0);
  auto ctx = makeContext(pixels);
  pipeline.render(ctx);

  EXPECT_EQ(pixels[0], 15);
  EXPECT_EQ(pixels[1], 0);
}

TEST(PipelineTest, DeterministicOutputWithFixedSeedAndAudio) {
  avs::core::EffectRegistry registry;
  registry.registerFactory("noise", []() { return std::make_unique<NoiseEffect>(); });

  ASSERT_EQ(setenv("AVS_SEED", "1337", 1), 0);

  auto buildPipeline = [&registry]() {
    avs::core::Pipeline pipeline(registry);
    pipeline.add("noise", ParamBlock{});
    return pipeline;
  };

  std::vector<std::uint8_t> pixelsA(64, 0);
  auto ctxA = makeContext(pixelsA);
  std::vector<std::uint8_t> pixelsB(64, 0);
  auto ctxB = makeContext(pixelsB);

  auto pipelineA = buildPipeline();
  auto pipelineB = buildPipeline();

  pipelineA.render(ctxA);
  pipelineB.render(ctxB);

  EXPECT_EQ(hashFNV1a(pixelsA), hashFNV1a(pixelsB));

  unsetenv("AVS_SEED");
}

TEST(PipelineTest, StopsProcessingWhenEffectFails) {
  avs::core::EffectRegistry registry;
  bool failingExecuted = false;
  bool succeedingExecuted = false;

  registry.registerFactory("fail", [&failingExecuted]() {
    return std::make_unique<FlagEffect>(false, &failingExecuted);
  });
  registry.registerFactory("succeed", [&succeedingExecuted]() {
    return std::make_unique<FlagEffect>(true, &succeedingExecuted);
  });

  avs::core::Pipeline pipeline(registry);
  pipeline.add("fail", ParamBlock{});
  pipeline.add("succeed", ParamBlock{});

  std::vector<std::uint8_t> pixels(4u, 0u);
  auto ctx = makeContext(pixels);
  const bool result = pipeline.render(ctx);

  EXPECT_FALSE(result);
  EXPECT_TRUE(failingExecuted);
  EXPECT_FALSE(succeedingExecuted);
}

