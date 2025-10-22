#include <gtest/gtest.h>
#if AVS_BUILD_AUDIO
#include <portaudio.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

#if AVS_BUILD_AUDIO
#include "avs/audio_portaudio_internal.hpp"
#endif

#include "avs/effects.hpp"
#include "avs/effects_render.hpp"
#include "avs/engine.hpp"
#include "avs/fs.hpp"
#include "avs/preset.hpp"
#include "avs/registry.hpp"

#if __has_include(<portaudio.h>)
#include "avs/audio_portaudio_internal.hpp"
#define AVS_TEST_HAS_PORTAUDIO 1
#else
#define AVS_TEST_HAS_PORTAUDIO 0
#endif

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

namespace {

struct FrameView {
  std::vector<std::uint8_t> data;
  FrameBufferView view;
};

FrameView makeFrameView(int width, int height) {
  FrameView fv;
  fv.data.assign(static_cast<std::size_t>(width) * height * 4u, 0);
  fv.view.data = fv.data.data();
  fv.view.width = width;
  fv.view.height = height;
  fv.view.stride = width * 4;
  return fv;
}

struct RenderFixture {
  TimingInfo timing{};
  AudioFeatures audio{};
  FrameBuffers buffers{};
  RNG rng{};
  ProcessContext ctx;

  RenderFixture(FrameBufferView& current, FrameBufferView& previous)
      : buffers{current, previous}, ctx{timing, audio, buffers, &rng, nullptr} {
    timing.deterministic = true;
    timing.fps_hint = 60;
  }
};

std::string hashBytes(const std::vector<std::uint8_t>& data) {
  constexpr std::uint64_t kOffset = 1469598103934665603ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffset;
  for (std::uint8_t byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
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

TEST(PresetParser, ParsesLegacyBinaryPreset) {
  namespace fs = std::filesystem;
  auto source = fs::path(SOURCE_DIR) / "tests/data/simple.avs";
  std::ifstream in(source, std::ios::binary);
  ASSERT_TRUE(in);
  std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  const std::string prefix = "Nullsoft AVS Preset ";
  auto prefixPos = std::search(data.begin(), data.end(), prefix.begin(), prefix.end());
  ASSERT_NE(prefixPos, data.end());
  auto versionBegin = prefixPos;
  std::advance(versionBegin, static_cast<std::vector<char>::difference_type>(prefix.size()));
  auto versionEnd = std::find(versionBegin, data.end(), '\x1a');
  ASSERT_NE(versionEnd, data.end());
  ASSERT_GE(std::distance(versionBegin, versionEnd), 3);
  const std::array<char, 3> legacyVersion{'0', '.', '1'};
  std::vector<char> legacy = data;
  auto versionIndex = std::distance(data.begin(), versionBegin);
  std::copy(legacyVersion.begin(), legacyVersion.end(), legacy.begin() + versionIndex);
  auto tmp = fs::temp_directory_path() / "legacy_preset.avs";
  {
    std::ofstream out(tmp, std::ios::binary);
    ASSERT_TRUE(out);
    out.write(legacy.data(), static_cast<std::streamsize>(legacy.size()));
  }
  auto preset = avs::parsePreset(tmp);
  EXPECT_TRUE(preset.warnings.empty());
  ASSERT_EQ(preset.chain.size(), 1u);
  auto* scripted = dynamic_cast<avs::ScriptedEffect*>(preset.chain[0].get());
  ASSERT_NE(scripted, nullptr);
  EXPECT_EQ(scripted->frameScript(), "red=bass; green=mid; blue=treb;");
  EXPECT_EQ(scripted->pixelScript(), "red=red; green=green; blue=blue;");
  fs::remove(tmp);
}

TEST(PresetParser, WarnsWhenEffectIsUndecodedButRecognized) {
  namespace fs = std::filesystem;
  auto preset = avs::parsePreset(fs::path(SOURCE_DIR) / "tests/data/phase1/fadeout_color.avs");
  ASSERT_FALSE(preset.warnings.empty());
  const std::string& warning = preset.warnings.front();
  EXPECT_NE(warning.find("preset loader does not yet decode effect: 3 (Trans / Fadeout)"),
            std::string::npos);
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

TEST(PresetParser, ReadsFirstEffectAfterExtendedHeader) {
  namespace fs = std::filesystem;
  auto preset = avs::parsePreset(fs::path(SOURCE_DIR) / "tests/data/extended_header.avs");
  EXPECT_TRUE(preset.warnings.empty());
  ASSERT_EQ(preset.comments.size(), 1u);
  EXPECT_EQ(preset.comments[0], "Hello!");
  EXPECT_TRUE(preset.unknown.empty());
}

TEST(PresetParser, HandlesExtendedRenderListHeader) {
  namespace fs = std::filesystem;
  auto preset = avs::parsePreset(fs::path(SOURCE_DIR) / "tests/data/extended_color_mod.avs");
  EXPECT_TRUE(preset.warnings.empty());
  ASSERT_FALSE(preset.chain.empty());
  auto* scripted = dynamic_cast<avs::ScriptedEffect*>(preset.chain.front().get());
  ASSERT_NE(scripted, nullptr);

  avs::Engine engine(32, 24);
  engine.setChain(std::move(preset.chain));
  engine.setAudio(avs::AudioState{});
  EXPECT_NO_THROW(engine.step(0.0f));
  EXPECT_EQ(engine.frame().w, 32);
  EXPECT_EQ(engine.frame().h, 24);
  EXPECT_EQ(engine.frame().rgba.size(), static_cast<size_t>(32 * 24 * 4));
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

#if AVS_BUILD_PLATFORM
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
#endif  // AVS_BUILD_PLATFORM


#if AVS_TEST_HAS_PORTAUDIO

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

TEST(PortAudioCallback, FlagsReportUnderflowEvents) {
  int dummy = 0;
  EXPECT_FALSE(avs::portaudio_detail::callbackIndicatesUnderflow(&dummy, paNoFlag));
  EXPECT_TRUE(avs::portaudio_detail::callbackIndicatesUnderflow(nullptr, paNoFlag));
  EXPECT_TRUE(avs::portaudio_detail::callbackIndicatesUnderflow(&dummy, paInputUnderflow));
}

TEST(PortAudioNegotiation, FallsBackToDefaultRate) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.requestedSampleRate = 44100;
  request.requestedChannels = 2;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 2, 2};
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

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 4, 4};
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

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 2, 2};
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

TEST(PortAudioNegotiation, UsesDeviceDefaultSampleRateWhenRequested) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.useDeviceDefaultSampleRate = true;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{44100.0, 2, 1};
  int queryCount = 0;
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        ++queryCount;
        EXPECT_EQ(channels, 2);
        return std::fabs(rate - 44100.0) < 1e-6;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_FALSE(result.usedFallbackRate);
  EXPECT_DOUBLE_EQ(result.sampleRate, 44100.0);
  EXPECT_EQ(result.channelCount, 2);
  EXPECT_EQ(queryCount, 1);
}

TEST(PortAudioNegotiation, PrefersEngineRateWhenDeviceDefaultDisabled) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.useDeviceDefaultSampleRate = false;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{44100.0, 2, 2};
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        EXPECT_EQ(channels, 2);
        return std::fabs(rate - 48000.0) < 1e-6;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_FALSE(result.usedFallbackRate);
  EXPECT_DOUBLE_EQ(result.sampleRate, 48000.0);
}

TEST(PortAudioNegotiation, UsesDeviceDefaultChannelsWhenRequested) {
  avs::portaudio_detail::StreamNegotiationRequest request;
  request.engineSampleRate = 48000;
  request.engineChannels = 2;
  request.useDeviceDefaultChannels = true;

  avs::portaudio_detail::StreamNegotiationDeviceInfo device{48000.0, 2, 1};
  auto result =
      avs::portaudio_detail::negotiateStream(request, device, [&](int channels, double rate) {
        EXPECT_EQ(rate, 48000.0);
        return channels == 1;
      });

  EXPECT_TRUE(result.supported);
  EXPECT_EQ(result.channelCount, 1);
  EXPECT_FALSE(result.usedFallbackRate);
}

TEST(PortAudioDeviceSelection, ParsesNumericIdentifier) {
  std::vector<avs::portaudio_detail::DeviceSummary> devices = {
      {0, "Built-in Mic", 2},
      {2, "USB Microphone", 1},
  };

  auto result = avs::portaudio_detail::resolveInputDeviceIdentifier(std::optional<std::string>{"2"},
                                                                    3, devices);

  ASSERT_TRUE(result.index.has_value());
  EXPECT_EQ(*result.index, 2);
  EXPECT_TRUE(result.error.empty());
}

TEST(PortAudioDeviceSelection, MatchesSubstringCaseInsensitive) {
  std::vector<avs::portaudio_detail::DeviceSummary> devices = {
      {0, "Analog Capture", 2},
      {1, "USB Podcast Interface", 2},
  };

  auto result = avs::portaudio_detail::resolveInputDeviceIdentifier(
      std::optional<std::string>{"podcast"}, 2, devices);

  ASSERT_TRUE(result.index.has_value());
  EXPECT_EQ(*result.index, 1);
  EXPECT_TRUE(result.error.empty());
}

TEST(PortAudioDeviceSelection, RejectsCapturelessDevice) {
  std::vector<avs::portaudio_detail::DeviceSummary> devices = {
      {4, "Loopback", 0},
  };

  auto result = avs::portaudio_detail::resolveInputDeviceIdentifier(std::optional<std::string>{"4"},
                                                                    5, devices);

  EXPECT_FALSE(result.index.has_value());
  EXPECT_FALSE(result.error.empty());
  EXPECT_NE(result.error.find("cannot capture audio"), std::string::npos);
}

#endif  // AVS_TEST_HAS_PORTAUDIO

namespace {

double legacyGetVis(const std::uint8_t* base, size_t sampleCount, int channels, double band,
                    double bandw, int ch, int xorv) {
  if (!base || sampleCount == 0) {
    return 0.0;
  }
  if (ch && ch != 1 && ch != 2) {
    return 0.0;
  }
  const int count = static_cast<int>(sampleCount);
  int bc = static_cast<int>(band * static_cast<double>(count));
  int bw = static_cast<int>(bandw * static_cast<double>(count));
  if (bw < 1) bw = 1;
  bc -= bw / 2;
  if (bc < 0) {
    bw += bc;
    bc = 0;
  }
  if (bc > count - 1) bc = count - 1;
  if (bc + bw > count) bw = count - bc;
  if (bw <= 0) {
    return 0.0;
  }
  const std::uint8_t* ch0 = base;
  const std::uint8_t* ch1 = channels > 1 ? base + sampleCount : nullptr;
  if (ch == 0) {
    double accum = 0.0;
    const double denom = (channels > 1 ? 255.0 : 127.5) * static_cast<double>(bw);
    if (!denom) return 0.0;
    for (int x = 0; x < bw; ++x) {
      accum += static_cast<double>((ch0[bc + x] ^ xorv) - xorv);
      if (channels > 1 && ch1) {
        accum += static_cast<double>((ch1[bc + x] ^ xorv) - xorv);
      } else if (channels <= 1 && xorv != 0) {
        accum += static_cast<double>((ch0[bc + x] ^ xorv) - xorv);
      }
    }
    return accum / denom;
  }
  const std::uint8_t* src = ch == 2 ? ch1 : ch0;
  if (!src) {
    return 0.0;
  }
  double accum = 0.0;
  for (int x = 0; x < bw; ++x) {
    accum += static_cast<double>((src[bc + x] ^ xorv) - xorv);
  }
  const double denom = 127.5 * static_cast<double>(bw);
  return denom != 0.0 ? accum / denom : 0.0;
}

void configureVm(avs::EelVm& vm,
                 const std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2>& osc,
                 const std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2>& spec,
                 double audioTime, double engineTime, const avs::MouseState& mouse) {
  avs::EelVm::LegacySources sources{};
  sources.oscBase = osc.data();
  sources.specBase = spec.data();
  sources.sampleCount = avs::EelVm::kLegacyVisSamples;
  sources.channels = 2;
  sources.audioTimeSeconds = audioTime;
  sources.engineTimeSeconds = engineTime;
  sources.mouse = mouse;
  vm.setLegacySources(sources);
}

std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2> makeOscSamples() {
  std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2> data{};
  const size_t count = avs::EelVm::kLegacyVisSamples;
  for (size_t i = 0; i < count; ++i) {
    data[i] = static_cast<std::uint8_t>((i * 3) % 256);
    data[i + count] = static_cast<std::uint8_t>((255 - (i * 5) % 256));
  }
  return data;
}

std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2> makeSpecSamples() {
  std::array<std::uint8_t, avs::EelVm::kLegacyVisSamples * 2> data{};
  const size_t count = avs::EelVm::kLegacyVisSamples;
  for (size_t i = 0; i < count; ++i) {
    data[i] = static_cast<std::uint8_t>((i * 7) % 256);
    data[i + count] = static_cast<std::uint8_t>((i * 11) % 256);
  }
  return data;
}

}  // namespace

TEST(EelVmBuiltins, GetOscMatchesLegacy) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{0.1, -0.2, true, false, true};
  avs::EelVm vm;
  configureVm(vm, osc, spec, 5.0, 10.0, mouse);

  EEL_F* result = vm.regVar("result");
  auto code = vm.compile("result = getosc(0.25, 0.1, 1);\n");
  vm.execute(code);
  double expected = legacyGetVis(osc.data(), avs::EelVm::kLegacyVisSamples, 2, 0.25, 0.1, 1, 128);
  EXPECT_NEAR(*result, expected, 1e-6);
  vm.freeCode(code);
}

TEST(EelVmBuiltins, GetSpecMatchesLegacy) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{};
  avs::EelVm vm;
  configureVm(vm, osc, spec, 3.0, 4.0, mouse);

  EEL_F* result = vm.regVar("result");
  auto code = vm.compile("result = getspec(0.3, 0.05, 0);\n");
  vm.execute(code);
  double expected =
      0.5 * legacyGetVis(spec.data(), avs::EelVm::kLegacyVisSamples, 2, 0.3, 0.05, 0, 0);
  EXPECT_NEAR(*result, expected, 1e-6);
  vm.freeCode(code);
}

TEST(EelVmBuiltins, GetTimeProvidesAudioAndDelta) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{};
  avs::EelVm vm;
  configureVm(vm, osc, spec, 2.5, 8.0, mouse);

  EEL_F* t1 = vm.regVar("t1");
  EEL_F* t2 = vm.regVar("t2");
  EEL_F* t3 = vm.regVar("t3");
  auto code = vm.compile("t1 = gettime(-1); t2 = gettime(-2); t3 = gettime(3);\n");
  vm.execute(code);
  EXPECT_NEAR(*t1, 2.5, 1e-9);
  EXPECT_NEAR(*t2, 2500.0, 1e-6);
  EXPECT_NEAR(*t3, 5.0, 1e-9);
  vm.freeCode(code);
}

TEST(EelVmBuiltins, GetKbMouseReflectsState) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{0.5, -0.75, true, false, true};
  avs::EelVm vm;
  configureVm(vm, osc, spec, 0.0, 0.0, mouse);

  EEL_F* mx = vm.regVar("mx");
  EEL_F* my = vm.regVar("my");
  EEL_F* ml = vm.regVar("ml");
  EEL_F* mr = vm.regVar("mr");
  EEL_F* mm = vm.regVar("mm");
  auto code = vm.compile(
      "mx = getkbmouse(1); my = getkbmouse(2); ml = getkbmouse(3); mr = getkbmouse(4); mm = "
      "getkbmouse(5);\n");
  vm.execute(code);
  EXPECT_NEAR(*mx, 0.5, 1e-9);
  EXPECT_NEAR(*my, -0.75, 1e-9);
  EXPECT_EQ(*ml, 1.0);
  EXPECT_EQ(*mr, 0.0);
  EXPECT_EQ(*mm, 1.0);
  vm.freeCode(code);
}

TEST(EelVmBuiltins, MegaBufIsPerVm) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{};
  avs::EelVm vm1;
  configureVm(vm1, osc, spec, 0.0, 0.0, mouse);
  avs::EelVm vm2;
  configureVm(vm2, osc, spec, 0.0, 0.0, mouse);

  EEL_F* res1 = vm1.regVar("res");
  auto code1 = vm1.compile("megabuf(5) = 1; res = megabuf(5);\n");
  vm1.execute(code1);
  vm1.freeCode(code1);
  EXPECT_EQ(*res1, 1.0);

  EEL_F* res2 = vm2.regVar("res");
  auto code2 = vm2.compile("res = megabuf(5);\n");
  vm2.execute(code2);
  vm2.freeCode(code2);
  EXPECT_EQ(*res2, 0.0);
}

TEST(EelVmBuiltins, GMegaBufIsSharedAcrossVms) {
  auto osc = makeOscSamples();
  auto spec = makeSpecSamples();
  avs::MouseState mouse{};
  avs::EelVm vm1;
  configureVm(vm1, osc, spec, 0.0, 0.0, mouse);
  avs::EelVm vm2;
  configureVm(vm2, osc, spec, 0.0, 0.0, mouse);

  auto setCode = vm1.compile("gmegabuf(200) = 42;\n");
  vm1.execute(setCode);
  vm1.freeCode(setCode);

  EEL_F* res = vm2.regVar("res");
  auto readCode = vm2.compile("res = gmegabuf(200);\n");
  vm2.execute(readCode);
  vm2.freeCode(readCode);
  EXPECT_EQ(*res, 42.0);
}

TEST(RenderGeometryEffects, ShapesCircleSnapshot) {
  auto frame = makeFrameView(64, 64);
  auto prev = makeFrameView(64, 64);
  RenderFixture fixture(frame.view, prev.view);
  ShapesEffect effect;
  effect.set_parameter("shape", ParamValue(std::string("circle")));
  effect.set_parameter("x", ParamValue(32));
  effect.set_parameter("y", ParamValue(32));
  effect.set_parameter("radius", ParamValue(18));
  effect.set_parameter("color", ParamValue(ColorRGBA8{200, 100, 40, 255}));
  effect.set_parameter("outlinecolor", ParamValue(ColorRGBA8{255, 255, 255, 255}));
  effect.set_parameter("outlinewidth", ParamValue(2));
  effect.process(fixture.ctx, frame.view);
  EXPECT_EQ(hashBytes(frame.data), "dcce202fe84d403e");
}

TEST(RenderGeometryEffects, TrianglesFilledSnapshot) {
  auto frame = makeFrameView(72, 72);
  auto prev = makeFrameView(72, 72);
  RenderFixture fixture(frame.view, prev.view);
  TrianglesEffect effect;
  effect.set_parameter("triangles", ParamValue(std::string("16,12 56,16 20,60")));
  effect.set_parameter("color", ParamValue(ColorRGBA8{20, 200, 220, 255}));
  effect.set_parameter("outlinecolor", ParamValue(ColorRGBA8{0, 0, 0, 255}));
  effect.set_parameter("outlinewidth", ParamValue(3));
  effect.process(fixture.ctx, frame.view);
  EXPECT_EQ(hashBytes(frame.data), "8598a1b7ef3b611a");
}

TEST(RenderGeometryEffects, DotGridAlternatingSnapshot) {
  auto frame = makeFrameView(80, 80);
  auto prev = makeFrameView(80, 80);
  RenderFixture fixture(frame.view, prev.view);
  DotGridEffect effect;
  effect.set_parameter("cols", ParamValue(5));
  effect.set_parameter("rows", ParamValue(5));
  effect.set_parameter("spacing_x", ParamValue(12));
  effect.set_parameter("spacing_y", ParamValue(12));
  effect.set_parameter("offset_x", ParamValue(10));
  effect.set_parameter("offset_y", ParamValue(10));
  effect.set_parameter("radius", ParamValue(3));
  effect.set_parameter("color", ParamValue(ColorRGBA8{255, 180, 0, 255}));
  effect.set_parameter("alt_color", ParamValue(ColorRGBA8{40, 120, 255, 255}));
  effect.set_parameter("alternate", ParamValue(true));
  effect.process(fixture.ctx, frame.view);
  EXPECT_EQ(hashBytes(frame.data), "70b8a4f73ec541b5");
}

TEST(RenderGeometryEffects, TextOutlineShadowSnapshot) {
  auto frame = makeFrameView(96, 48);
  auto prev = makeFrameView(96, 48);
  RenderFixture fixture(frame.view, prev.view);
  TextEffect effect;
  effect.set_parameter("text", ParamValue(std::string("AVS")));
  effect.set_parameter("x", ParamValue(48));
  effect.set_parameter("y", ParamValue(24));
  effect.set_parameter("size", ParamValue(20));
  effect.set_parameter("color", ParamValue(ColorRGBA8{0, 120, 255, 255}));
  effect.set_parameter("outlinecolor", ParamValue(ColorRGBA8{0, 0, 0, 255}));
  effect.set_parameter("outlinesize", ParamValue(2));
  effect.set_parameter("shadow", ParamValue(true));
  effect.set_parameter("shadowcolor", ParamValue(ColorRGBA8{0, 0, 0, 180}));
  effect.set_parameter("shadowoffsetx", ParamValue(3));
  effect.set_parameter("shadowoffsety", ParamValue(3));
  effect.set_parameter("shadowblur", ParamValue(2));
  effect.set_parameter("halign", ParamValue(std::string("center")));
  effect.set_parameter("valign", ParamValue(std::string("middle")));
  effect.process(fixture.ctx, frame.view);
  EXPECT_EQ(hashBytes(frame.data), "8163c4119c3af4fb");
}

TEST(RenderGeometryEffects, SuperscopeSineWaveSnapshot) {
  auto frame = makeFrameView(96, 96);
  auto prev = makeFrameView(96, 96);
  RenderFixture fixture(frame.view, prev.view);
  SuperscopeEffect effect;
  effect.set_parameter("init", ParamValue(std::string("n=64; linesize=2; drawmode=1;")));
  effect.set_parameter("point", ParamValue(std::string("x=cos(i*6.2831853); y=sin(i*6.2831853); red=0.6; green=0.8; blue=0.2;")));
  InitContext initCtx;
  initCtx.frame_size = FrameSize{frame.view.width, frame.view.height};
  initCtx.deterministic = true;
  effect.init(initCtx);
  effect.process(fixture.ctx, frame.view);
  EXPECT_EQ(hashBytes(frame.data), "a7ea528ce862219f");
}

