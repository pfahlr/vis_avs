#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <avs/core/IEffect.hpp>
#include <avs/core/ParamBlock.hpp>
#include <avs/core/RenderContext.hpp>

namespace avs::audio {
struct Analysis;
}

namespace avs::effects::render {

/**
 * @brief Pulsing radial ring driven by the current audio waveform or spectrum.
 */
class Ring : public avs::core::IEffect {
 public:
  Ring();
  ~Ring() override = default;

  enum class Channel { Left, Right, Mix };
  enum class Source { Oscilloscope, Spectrum };
  enum class Placement { Left, Center, Right };

  void setParams(const avs::core::ParamBlock& params) override;
  bool render(avs::core::RenderContext& context) override;

 private:
  struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
  };

  struct Range {
    std::size_t begin = 0;
    std::size_t count = 0;
  };

  static constexpr int kSegments = 80;
  static constexpr int kColorCycle = 64;

  static int clampSize(int value);
  static std::string toLower(std::string_view value);
  static bool parseColorToken(std::string_view token, Color& color);
  static Color colorFromInt(std::uint32_t value);
  static void drawLine(avs::core::RenderContext& context, int x0, int y0, int x1, int y1,
                       const Color& color);
  static int mirroredSegment(int segment);
  static float samplePosition(int segment);

  void parseEffectBits(int effectBits);
  void parseChannelParam(const avs::core::ParamBlock& params);
  void parsePlacementParam(const avs::core::ParamBlock& params);
  void parseSourceParam(const avs::core::ParamBlock& params);
  void parseColorParams(const avs::core::ParamBlock& params);

  Range waveformRange(std::size_t total) const;
  Range spectrumRange(std::size_t total) const;
  float sampleWaveform(const avs::audio::Analysis& analysis, Range range, int segment) const;
  float sampleSpectrum(const avs::audio::Analysis& analysis, Range range, float rangeMax,
                       int segment) const;
  float sampleNormalized(const avs::audio::Analysis& analysis, Range waveform, Range spectrum,
                         float spectrumMax, int segment) const;

  Color currentColor() const;
  void normalizeColorCursor();

  Channel channel_ = Channel::Mix;
  Source source_ = Source::Oscilloscope;
  Placement placement_ = Placement::Center;
  int size_ = 8;
  int colorCursor_ = 0;
  std::vector<Color> colors_;
};

}  // namespace avs::effects::render
