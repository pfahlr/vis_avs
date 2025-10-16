#include "avs/core/DeterministicRng.hpp"

#include <cstdlib>

namespace {

std::uint64_t readSeedFromEnv() {
  const char* value = std::getenv("VIS_AVS_SEED");
  if (!value) {
    value = std::getenv("AVS_SEED");
  }
  if (!value) {
    return 0;
  }
  char* end = nullptr;
  unsigned long long parsed = std::strtoull(value, &end, 10);
  if (end == value) {
    return 0;
  }
  return static_cast<std::uint64_t>(parsed);
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
