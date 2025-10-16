#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "avs/runtime/framebuffers.h"
#include "md5_helper.hpp"

namespace {

using avs::runtime::BufferSlot;
using avs::runtime::ClearBlendMode;
using avs::runtime::ClearSettings;
using avs::runtime::Framebuffers;
using avs::runtime::PersistSettings;
using avs::runtime::SlideDirection;
using avs::runtime::SlideSettings;
using avs::runtime::TransitionSettings;
using avs::runtime::WrapSettings;

std::filesystem::path goldenDir() {
  return std::filesystem::path(SOURCE_DIR) / "tests" / "presets" / "fb_ops";
}

std::vector<std::string> readGolden(const std::string& name) {
  std::filesystem::path path = goldenDir() / (name + ".md5");
  std::ifstream file(path);
  if (!file) {
    ADD_FAILURE() << "Missing golden file: " << path;
    return {};
  }
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

std::string hashFrame(const avs::runtime::FrameView& view) {
  if (!view.data) return {};
  const auto span = view.span();
  return fb_ops_test::computeMd5Hex(span.data(), span.size());
}

void fillPattern(const avs::runtime::FrameView& view, int seed) {
  if (!view.data) return;
  for (int y = 0; y < view.height; ++y) {
    std::uint8_t* row = view.data + y * view.stride;
    for (int x = 0; x < view.width; ++x) {
      std::uint8_t* px = row + x * 4;
      px[0] = static_cast<std::uint8_t>((seed * 13 + x * 7 + y * 3) & 0xFF);
      px[1] = static_cast<std::uint8_t>((seed * 17 + x * 5 + y * 11) & 0xFF);
      px[2] = static_cast<std::uint8_t>((seed * 29 + x * 3 + y * 2) & 0xFF);
      px[3] = 255;
    }
  }
}

std::string joinLines(const std::vector<std::string>& values) {
  std::string out;
  for (const auto& v : values) {
    out += v;
    out.push_back('\n');
  }
  return out;
}

void expectHashesEqual(const std::vector<std::string>& expected,
                       const std::vector<std::string>& actual) {
  if (expected.empty() && !actual.empty()) {
    std::cout << "Captured hashes:\n" << joinLines(actual);
  }
  ASSERT_EQ(expected.size(), actual.size())
      << "Expected " << expected.size() << " hashes but got " << actual.size();
  if (expected != actual) {
    ADD_FAILURE() << "Golden mismatch\nExpected:\n" << joinLines(expected)
                  << "Got:\n" << joinLines(actual);
  }
}

}  // namespace

TEST(FrameBufferOps, SaveRestoreRoundTrip) {
  Framebuffers fb(8, 6);
  fillPattern(fb.currentView(), -1);

  std::vector<std::string> hashes;
  for (int frame = 0; frame < 10; ++frame) {
    fb.beginFrame();
    fillPattern(fb.currentView(), frame);

    ClearSettings clearSettings{};
    clearSettings.argb = 0xFF101010u;
    clearSettings.blend = ClearBlendMode::Replace;
    clearSettings.firstFrameOnly = true;

    avs::runtime::effect_save(fb, BufferSlot::A);
    avs::runtime::effect_clear(fb, clearSettings);
    avs::runtime::effect_restore(fb, BufferSlot::A);

    fb.finishFrame();
    hashes.push_back(hashFrame(fb.currentView()));
  }

  expectHashesEqual(readGolden("save_restore"), hashes);
}

TEST(FrameBufferOps, WrapAroundOffsets) {
  Framebuffers fb(7, 5);
  fillPattern(fb.currentView(), 42);

  std::vector<std::string> hashes;
  for (int frame = 0; frame < 10; ++frame) {
    fb.beginFrame();
    fillPattern(fb.previousView(), frame * 3);
    const int dx = (frame % 5) - 2;
    const int dy = ((frame * 2) % 7) - 3;
    WrapSettings wrap{dx, dy};
    avs::runtime::effect_wrap(fb, wrap);
    fb.finishFrame();
    hashes.push_back(hashFrame(fb.currentView()));
  }

  expectHashesEqual(readGolden("wrap"), hashes);
}

TEST(FrameBufferOps, PersistentOverlaysFadeOverFrames) {
  Framebuffers fb(8, 6);
  fillPattern(fb.currentView(), 0);

  PersistSettings title{6, {240, 32, 32, 255}};
  PersistSettings text1{4, {32, 240, 32, 255}};
  PersistSettings text2{5, {32, 32, 240, 255}};

  std::vector<std::string> hashes;
  for (int frame = 0; frame < 10; ++frame) {
    fb.beginFrame();
    ClearSettings clearSettings{};
    clearSettings.argb = 0xFF000000u;
    clearSettings.blend = ClearBlendMode::Replace;
    avs::runtime::effect_clear(fb, clearSettings);

    if (frame == 0) {
      avs::runtime::effect_persist_title(fb, title);
    }
    if (frame == 2) {
      avs::runtime::effect_persist_text1(fb, text1);
    }
    if (frame == 4) {
      avs::runtime::effect_persist_text2(fb, text2);
    }

    if (frame == 6) {
      // Trigger another title overlay to ensure restart works.
      PersistSettings flash{3, {200, 200, 200, 255}};
      avs::runtime::effect_persist_title(fb, flash);
    }

    fb.finishFrame();
    hashes.push_back(hashFrame(fb.currentView()));
  }

  expectHashesEqual(readGolden("persist"), hashes);
}

TEST(FrameBufferOps, SlideAndTransitionBlend) {
  Framebuffers fb(9, 7);
  fillPattern(fb.currentView(), 1);

  std::vector<std::string> hashes;
  for (int frame = 0; frame < 10; ++frame) {
    fb.beginFrame();
    // Prepare source image for transitions.
    fillPattern(fb.previousView(), frame + 5);
    fillPattern(fb.currentView(), frame * 11);

    SlideSettings slideIn{SlideDirection::Left, 1 + (frame % 3)};
    avs::runtime::effect_in_slide(fb, slideIn);
    SlideSettings slideOut{SlideDirection::Down, frame % 2 + 1};
    avs::runtime::effect_out_slide(fb, slideOut);

    TransitionSettings transition{std::clamp(frame / 9.0f, 0.0f, 1.0f)};
    avs::runtime::effect_transition(fb, transition);

    fb.finishFrame();
    hashes.push_back(hashFrame(fb.currentView()));
  }

  expectHashesEqual(readGolden("slide_transition"), hashes);
}

