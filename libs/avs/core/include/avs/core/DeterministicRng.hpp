#pragma once

#include <cstdint>
#include <random>

namespace avs::core {

/**
 * @brief Frame-level deterministic random number generator.
 */
class DeterministicRng {
 public:
  DeterministicRng();
  explicit DeterministicRng(std::uint64_t seed);

  /**
   * @brief Update the RNG state for a specific frame index.
   */
  void reseed(std::uint64_t frameIndex);

  /**
   * @brief Generate the next raw 32-bit value.
   */
  [[nodiscard]] std::uint32_t nextUint32();

  /**
   * @brief Uniform floating point distribution helper.
   */
  [[nodiscard]] float uniform(float min, float max);

  [[nodiscard]] std::uint64_t seed() const { return baseSeed_; }

 private:
  std::uint64_t baseSeed_ = 0;
  std::mt19937 engine_;
};

}  // namespace avs::core
