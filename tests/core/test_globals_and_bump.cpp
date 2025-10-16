#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

#include "avs/core/IEffect.hpp"
#include "avs/core/ParamBlock.hpp"
#include "avs/core/Pipeline.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/effects/RegisterEffects.hpp"
#include "avs/runtime/GlobalState.hpp"

namespace {

using avs::core::ParamBlock;
using avs::core::RenderContext;

RenderContext makeContext(std::vector<std::uint8_t>& pixels, avs::runtime::GlobalState& globals) {
  RenderContext ctx;
  ctx.width = 4;
  ctx.height = 1;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.globals = &globals;
  ctx.deltaSeconds = 1.0 / 60.0;
  return ctx;
}

class CaptureEffect : public avs::core::IEffect {
 public:
  explicit CaptureEffect(std::vector<double>* values) : values_(values) {}

  bool render(RenderContext& context) override {
    if (context.globals && values_) {
      values_->push_back(context.globals->registers[0]);
      values_->push_back(context.globals->registers[1]);
    }
    return true;
  }

  void setParams(const ParamBlock&) override {}

 private:
  std::vector<double>* values_;
};

}  // namespace

TEST(GlobalsEffectTest, SharedRegistersAcrossChain) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  std::vector<double> observed;
  registry.registerFactory("capture", [&observed]() { return std::make_unique<CaptureEffect>(&observed); });

  avs::core::Pipeline pipeline(registry);

  ParamBlock first;
  first.setString("frame", "g1 = g1 + 1;" );
  pipeline.add("globals", first);

  ParamBlock second;
  second.setString("frame", "g2 = g1;" );
  pipeline.add("globals", second);

  pipeline.add("capture", ParamBlock{});

  avs::runtime::GlobalState globals;
  std::vector<std::uint8_t> pixels(16, 0);
  auto ctx = makeContext(pixels, globals);

  for (int frame = 0; frame < 3; ++frame) {
    ctx.frameIndex = static_cast<std::uint64_t>(frame);
    ASSERT_TRUE(pipeline.render(ctx));
  }

  ASSERT_EQ(observed.size(), 6u);
  EXPECT_NEAR(observed[0], 1.0, 1e-6);
  EXPECT_NEAR(observed[1], 1.0, 1e-6);
  EXPECT_NEAR(observed[2], 2.0, 1e-6);
  EXPECT_NEAR(observed[3], 2.0, 1e-6);
  EXPECT_NEAR(observed[4], 3.0, 1e-6);
  EXPECT_NEAR(observed[5], 3.0, 1e-6);
}

TEST(BumpEffectTest, HorizontalDisplacementFromHeightmap) {
  avs::core::EffectRegistry registry;
  avs::effects::registerCoreEffects(registry);

  avs::core::Pipeline pipeline(registry);
  ParamBlock bumpParams;
  bumpParams.setBool("use_frame_heightmap", false);
  bumpParams.setString("heightmap", "wave");
  bumpParams.setFloat("scale_x", 2.0f);
  pipeline.add("bump", bumpParams);

  const int width = 8;
  const int height = 1;
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4), 0);
  for (int x = 0; x < width; ++x) {
    const std::size_t idx = static_cast<std::size_t>(x) * 4u;
    pixels[idx + 0u] = static_cast<std::uint8_t>(x * 10);
    pixels[idx + 3u] = 255u;
  }
  const auto original = pixels;

  avs::runtime::GlobalState globals;
  avs::runtime::Heightmap wave;
  wave.width = width;
  wave.height = height;
  wave.samples.resize(static_cast<std::size_t>(width * height));
  for (int x = 0; x < width; ++x) {
    const double phase = (static_cast<double>(x) / static_cast<double>(width)) *
                         2.0 * std::numbers::pi_v<double>;
    wave.samples[static_cast<std::size_t>(x)] = static_cast<float>(0.5 + 0.5 * std::sin(phase));
  }
  globals.heightmaps.emplace("wave", std::move(wave));

  RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.globals = &globals;

  ASSERT_TRUE(pipeline.render(ctx));

  auto redAt = [&](int x) {
    return pixels[static_cast<std::size_t>(x) * 4u];
  };

  EXPECT_EQ(redAt(0), original[0]);
  EXPECT_EQ(redAt(2), original[static_cast<std::size_t>(3) * 4u]);
  EXPECT_EQ(redAt(4), original[static_cast<std::size_t>(4) * 4u]);
  EXPECT_EQ(redAt(6), original[static_cast<std::size_t>(5) * 4u]);
}

