#include "avs/core/DeterministicRng.hpp"

#include <cstdlib>
#include <optional>

namespace {

std::optional<std::uint64_t> parseSeed(const char* value) {
  if (!value) {
    return std::nullopt;
  }
  char* end = nullptr;
  unsigned long long parsed = std::strtoull(value, &end, 10);
  if (end == value) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(parsed);
}

std::uint64_t readSeedFromEnv() {
  if (auto seed = parseSeed(std::getenv("VIS_AVS_SEED"))) {
    return *seed;
  }
  if (auto seed = parseSeed(std::getenv("AVS_SEED"))) {
    return *seed;
  }
  return 0;
}

}  // namespace

namespace avs::core {

DeterministicRng::DeterministicRng() : DeterministicRng(readSeedFromEnv()) {}

DeterministicRng::DeterministicRng(std::uint64_t seed) : baseSeed_(seed) {
  reseed(0);
}

void DeterministicRng::reseed(std::uint64_t frameIndex) {
  std::uint64_t combined = baseSeed_ ^ (frameIndex + 0x9E3779B97F4A7C15ull);
  engine_.seed(static_cast<std::mt19937::result_type>(combined));
}

std::uint32_t DeterministicRng::nextUint32() {
  return engine_();
}

float DeterministicRng::uniform(float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(engine_);
}

}  // namespace avs::core
