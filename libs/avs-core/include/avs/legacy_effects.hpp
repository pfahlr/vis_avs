#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "avs/effects.hpp"

namespace avs {

// Base class for legacy effects that currently act as pass-through stubs.
class LegacyDecodedEffect : public Effect {
 public:
  void process(const Framebuffer& in, Framebuffer& out) override { out = in; }
};

struct SimpleSpectrumConfig {
  std::uint32_t effectBits = 0;
  std::vector<std::uint32_t> palette;
};

class LegacySimpleSpectrumEffect : public LegacyDecodedEffect {
 public:
  explicit LegacySimpleSpectrumEffect(SimpleSpectrumConfig config) : config_(std::move(config)) {}

  const SimpleSpectrumConfig& config() const { return config_; }

 private:
  SimpleSpectrumConfig config_;
};

struct FadeoutConfig {
  std::uint32_t fadeLength = 0;
  std::uint32_t targetColor = 0;
};

class LegacyFadeoutEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyFadeoutEffect(FadeoutConfig config) : config_(config) {}

  const FadeoutConfig& config() const { return config_; }

 private:
  FadeoutConfig config_;
};

struct BlurConfig {
  std::uint32_t mode = 0;
  std::uint32_t roundMode = 0;
};

class LegacyBlurEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyBlurEffect(BlurConfig config) : config_(config) {}

  const BlurConfig& config() const { return config_; }

 private:
  BlurConfig config_;
};

struct MovingParticleConfig {
  std::uint32_t enabled = 0;
  std::uint32_t color = 0;
  std::uint32_t maxDistance = 0;
  std::uint32_t size = 0;
  std::uint32_t secondarySize = 0;
  std::uint32_t blendMode = 0;
};

class LegacyMovingParticleEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyMovingParticleEffect(MovingParticleConfig config) : config_(config) {}

  const MovingParticleConfig& config() const { return config_; }

 private:
  MovingParticleConfig config_;
};

struct RingConfig {
  std::uint32_t effectBits = 0;
  std::vector<std::uint32_t> palette;
  std::uint32_t size = 0;
  std::uint32_t sourceChannel = 0;
};

class LegacyRingEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyRingEffect(RingConfig config) : config_(std::move(config)) {}

  const RingConfig& config() const { return config_; }

 private:
  RingConfig config_;
};

struct MovementConfig {
  enum class ScriptEncoding { None, Legacy, Tagged };

  std::int32_t effect = 0;
  std::int32_t blend = 0;
  std::int32_t sourceMapped = 0;
  std::int32_t rectangular = 0;
  std::int32_t subpixel = 0;
  std::int32_t wrap = 0;
  bool rectangularFlagFromScript = false;
  ScriptEncoding scriptEncoding = ScriptEncoding::None;
  std::string script;
};

class LegacyMovementEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyMovementEffect(MovementConfig config) : config_(std::move(config)) {}

  const MovementConfig& config() const { return config_; }

 private:
  MovementConfig config_;
};

struct DotFountainConfig {
  std::int32_t rotationVelocity = 0;
  std::array<std::uint32_t, 5> colors{};
  std::int32_t angle = 0;
  float radius = 0.0f;
};

class LegacyDotFountainEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyDotFountainEffect(DotFountainConfig config) : config_(config) {}

  const DotFountainConfig& config() const { return config_; }

 private:
  DotFountainConfig config_;
};

struct MirrorConfig {
  std::uint32_t enabled = 0;
  std::uint32_t mode = 0;
  std::uint32_t onBeat = 0;
  std::uint32_t smooth = 0;
  std::uint32_t slower = 0;
};

class LegacyMirrorEffect : public LegacyDecodedEffect {
 public:
  explicit LegacyMirrorEffect(MirrorConfig config) : config_(config) {}

  const MirrorConfig& config() const { return config_; }

 private:
  MirrorConfig config_;
};

}  // namespace avs
