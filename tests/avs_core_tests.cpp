#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include "avs/effects.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"

using namespace avs;

TEST(BlurEffect, SpreadsLight) {
  Framebuffer in;
  in.w = 3;
  in.h = 1;
  in.rgba.assign(3 * 4, 0);
  in.rgba[4 + 0] = 255;
  in.rgba[4 + 1] = 255;
  in.rgba[4 + 2] = 255;
  in.rgba[4 + 3] = 255;
  Framebuffer out;
  BlurEffect blur(1);
  blur.init(in.w, in.h);
  blur.process(in, out);
  EXPECT_GT(out.rgba[0], 0);
  EXPECT_LT(out.rgba[0], out.rgba[4]);
}

TEST(ConvolutionEffect, PreservesConstantColor) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba.assign(2 * 2 * 4, 0);
  for (size_t i = 0; i < in.rgba.size(); i += 4) {
    in.rgba[i + 0] = 100;
    in.rgba[i + 1] = 100;
    in.rgba[i + 2] = 100;
    in.rgba[i + 3] = 255;
  }
  Framebuffer out;
  ConvolutionEffect conv;
  conv.init(in.w, in.h);
  conv.process(in, out);
  for (size_t i = 0; i < out.rgba.size(); i += 4) {
    EXPECT_EQ(out.rgba[i + 0], 100);
    EXPECT_EQ(out.rgba[i + 1], 100);
    EXPECT_EQ(out.rgba[i + 2], 100);
    EXPECT_EQ(out.rgba[i + 3], 255);
  }
}

TEST(ColorMapEffect, ProducesColor) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {128, 128, 128, 255};
  Framebuffer out;
  ColorMapEffect cm;
  cm.init(in.w, in.h);
  cm.process(in, out);
  EXPECT_NE(out.rgba[0], out.rgba[1]);
  EXPECT_EQ(out.rgba[3], 255);
}

TEST(PresetParser, ParsesChainAndReportsUnsupported) {
  auto tmp = std::filesystem::temp_directory_path() / "test.avs";
  {
    std::ofstream(tmp) << "blur radius=2\nunknown\n";
  }
  auto preset = avs::parsePreset(tmp);
  EXPECT_EQ(preset.chain.size(), 2u);
  EXPECT_TRUE(dynamic_cast<avs::BlurEffect*>(preset.chain[0].get()) != nullptr);
  EXPECT_EQ(preset.warnings.size(), 1u);
  std::filesystem::remove(tmp);
}

TEST(FileWatcher, DetectsModification) {
  auto tmp = std::filesystem::temp_directory_path() / "watch.txt";
  {
    std::ofstream(tmp) << "a";
  }
  avs::FileWatcher w(tmp);
  {
    std::ofstream(tmp) << "b";
  }
  bool changed = false;
  for (int i = 0; i < 10 && !changed; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    changed = w.poll();
  }
  EXPECT_TRUE(changed);
  std::filesystem::remove(tmp);
}
