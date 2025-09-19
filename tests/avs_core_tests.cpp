#include <gtest/gtest.h>

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <thread>

#include "avs/audio_portaudio_internal.hpp"
#include "avs/effects.hpp"
#include "avs/fs.hpp"
#include "avs/engine.hpp"
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

static std::uint32_t checksum(const Framebuffer& fb) {
  std::uint32_t sum = 0;
  for (auto v : fb.rgba) {
    sum += v;
  }
  return sum;
}

namespace {

std::string trimCopy(const std::string& s) {
  size_t begin = 0;
  while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin]))) {
    ++begin;
  }
  size_t end = s.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }
  return s.substr(begin, end - begin);
}

std::string hashFramebufferFNV(const Framebuffer& fb) {
  constexpr std::uint64_t kOffset = 1469598103934665603ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffset;
  for (std::uint8_t byte : fb.rgba) {
    hash ^= byte;
    hash *= kPrime;
  }
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

std::map<std::pair<int, int>, std::string> loadGoldenHashes(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::map<std::pair<int, int>, std::string> result;
  if (!file) {
    return result;
  }
  std::string line;
  while (std::getline(file, line)) {
    line = trimCopy(line);
    if (line.empty() || line[0] == '#') continue;
    std::istringstream iss(line);
    std::string sizeToken;
    std::string hash;
    if (!(iss >> sizeToken >> hash)) continue;
    size_t sep = sizeToken.find('x');
    if (sep == std::string::npos) continue;
    int w = std::stoi(sizeToken.substr(0, sep));
    int h = std::stoi(sizeToken.substr(sep + 1));
    result[{w, h}] = hash;
  }
  return result;
}

std::string renderPresetHash(std::vector<std::unique_ptr<Effect>> chain, int w, int h) {
  avs::Engine engine(w, h);
  engine.setChain(std::move(chain));
  engine.setAudio(avs::AudioState{});
  engine.step(0.0f);
  return hashFramebufferFNV(engine.frame());
}

}  // namespace

TEST(MotionBlurEffect, AveragesWithHistory) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {100, 0, 0, 50};
  Framebuffer out;
  MotionBlurEffect mb;
  mb.init(in.w, in.h);
  mb.process(in, out);
  EXPECT_EQ(checksum(out), 75u);
}

TEST(ColorTransformEffect, InvertsColor) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {10, 20, 30, 40};
  Framebuffer out;
  ColorTransformEffect ct;
  ct.init(in.w, in.h);
  ct.process(in, out);
  EXPECT_EQ(checksum(out), 920u);
}

TEST(GlowEffect, Brightens) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {200, 200, 200, 255};
  Framebuffer out;
  GlowEffect g;
  g.process(in, out);
  EXPECT_EQ(checksum(out), 1005u);
}

TEST(ZoomRotateEffect, Rotates180) {
  Framebuffer in;
  in.w = 2;
  in.h = 1;
  in.rgba = {1, 2, 3, 4, 5, 6, 7, 8};
  Framebuffer out;
  ZoomRotateEffect zr;
  zr.init(in.w, in.h);
  zr.process(in, out);
  ASSERT_EQ(out.rgba[0], 5);
  ASSERT_EQ(out.rgba[4], 1);
}

TEST(MirrorEffect, MirrorsHorizontally) {
  Framebuffer in;
  in.w = 2;
  in.h = 1;
  in.rgba = {1, 2, 3, 4, 5, 6, 7, 8};
  Framebuffer out;
  MirrorEffect m;
  m.init(in.w, in.h);
  m.process(in, out);
  ASSERT_EQ(out.rgba[0], 5);
  ASSERT_EQ(out.rgba[4], 1);
}

TEST(TunnelEffect, GeneratesGradient) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba.assign(2 * 2 * 4, 0);
  Framebuffer out;
  TunnelEffect t;
  t.init(in.w, in.h);
  t.process(in, out);
  EXPECT_EQ(checksum(out), 1038u);
}

TEST(RadialBlurEffect, AveragesWithCenter) {
  Framebuffer in;
  in.w = 2;
  in.h = 2;
  in.rgba = {0, 0, 0, 0, 10, 10, 10, 10, 20, 20, 20, 20, 30, 30, 30, 30};
  Framebuffer out;
  RadialBlurEffect rb;
  rb.init(in.w, in.h);
  rb.process(in, out);
  EXPECT_EQ(checksum(out), 360u);
}

TEST(AdditiveBlendEffect, AddsConstant) {
  Framebuffer in;
  in.w = 1;
  in.h = 1;
  in.rgba = {100, 100, 100, 100};
  Framebuffer out;
  AdditiveBlendEffect ab;
  ab.init(in.w, in.h);
  ab.process(in, out);
  EXPECT_EQ(checksum(out), 440u);
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

TEST(PresetParser, ParsesBinaryColorModifier) {
  auto preset = avs::parsePreset(std::filesystem::path(SOURCE_DIR) / "tests/data/simple.avs");
  ASSERT_EQ(preset.chain.size(), 1u);
  auto* scripted = dynamic_cast<avs::ScriptedEffect*>(preset.chain[0].get());
  ASSERT_NE(scripted, nullptr);
  EXPECT_TRUE(scripted->initScript().empty());
  EXPECT_TRUE(scripted->beatScript().empty());
  EXPECT_EQ(scripted->frameScript(), "red=bass; green=mid; blue=treb;");
  EXPECT_EQ(scripted->pixelScript(), "red=red; green=green; blue=blue;");
}

TEST(PresetParser, ParsesNestedRenderLists) {
  auto preset = avs::parsePreset(std::filesystem::path(SOURCE_DIR) / "tests/data/nested_list.avs");
  EXPECT_TRUE(preset.warnings.empty());
  ASSERT_EQ(preset.chain.size(), 1u);
  auto* composite = dynamic_cast<avs::CompositeEffect*>(preset.chain[0].get());
  ASSERT_NE(composite, nullptr);
  ASSERT_EQ(composite->childCount(), 1u);
  EXPECT_NE(dynamic_cast<avs::ScriptedEffect*>(composite->children()[0].get()), nullptr);
  ASSERT_EQ(preset.comments.size(), 1u);
  EXPECT_EQ(preset.comments[0], "Nested list comment");
}

TEST(ScriptedEffect, SuperscopeLegacyHashes) {
  namespace fs = std::filesystem;
  fs::path sourceDir{SOURCE_DIR};
  auto golden = loadGoldenHashes(sourceDir / "tests/golden/superscope_hashes.txt");
  ASSERT_FALSE(golden.empty());
  fs::path presetPath = sourceDir / "tests/data/superscope_classic.avs";
  for (const auto& dims : {std::pair<int, int>{32, 24}, std::pair<int, int>{64, 48}}) {
    auto parsed = avs::parsePreset(presetPath);
    EXPECT_TRUE(parsed.warnings.empty());
    ASSERT_FALSE(parsed.chain.empty());
    auto it = golden.find(dims);
    ASSERT_NE(it, golden.end());
    auto hash = renderPresetHash(std::move(parsed.chain), dims.first, dims.second);
    EXPECT_EQ(hash, it->second);
  }
}

TEST(ScriptedEffect, ColorModifierLegacyHashes) {
  namespace fs = std::filesystem;
  fs::path sourceDir{SOURCE_DIR};
  auto golden = loadGoldenHashes(sourceDir / "tests/golden/color_mod_hashes.txt");
  ASSERT_FALSE(golden.empty());
  fs::path presetPath = sourceDir / "tests/data/color_mod_classic.avs";
  for (const auto& dims : {std::pair<int, int>{16, 12}, std::pair<int, int>{48, 36}}) {
    auto parsed = avs::parsePreset(presetPath);
    EXPECT_TRUE(parsed.warnings.empty());
    ASSERT_FALSE(parsed.chain.empty());
    auto it = golden.find(dims);
    ASSERT_NE(it, golden.end());
    auto hash = renderPresetHash(std::move(parsed.chain), dims.first, dims.second);
    EXPECT_EQ(hash, it->second);
  }
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

TEST(PortAudioCallback, NullInputRaisesUnderflowFlag) {
  std::vector<float> ring(8, 1.0f);
  const size_t mask = ring.size() - 1;
  const size_t writeIndex = 2;
  const size_t samples = 4;

  auto result =
      avs::portaudio_detail::processCallbackInput(nullptr, samples, writeIndex, mask, ring);

  EXPECT_TRUE(result.underflow);
  EXPECT_EQ(result.nextWriteIndex, writeIndex + samples);
  for (size_t i = 0; i < samples; ++i) {
    EXPECT_FLOAT_EQ(ring[(writeIndex + i) & mask], 0.0f);
  }
}

TEST(PortAudioNegotiation, FallsBackToDefaultRate) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.requestedSampleRate = 44100;
  request.requestedChannels = 2;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 2};
  int queryCount = 0;
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        ++queryCount;
        EXPECT_EQ(channels, 2);
        return rate != 44100.0;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_TRUE(result.usedFallbackRate);
  EXPECT_DOUBLE_EQ(result.sampleRate, 48000.0);
  EXPECT_EQ(queryCount, 2);
}

TEST(PortAudioNegotiation, KeepsRequestedFormatWhenSupported) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.requestedSampleRate = 48000;
  request.requestedChannels = 2;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 4};
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        EXPECT_EQ(channels, 2);
        return rate == 48000.0;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_FALSE(result.usedFallbackRate);
  EXPECT_DOUBLE_EQ(result.sampleRate, 48000.0);
  EXPECT_EQ(result.channelCount, 2);
}

TEST(PortAudioNegotiation, ClampsRequestedChannelsToDeviceCapabilities) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.requestedChannels = 4;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 2};
  int queryCount = 0;
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        ++queryCount;
        EXPECT_EQ(rate, 48000.0);
        return channels == 2;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_EQ(result.channelCount, 2);
  EXPECT_FALSE(result.usedFallbackRate);
  EXPECT_EQ(queryCount, 1);
}
