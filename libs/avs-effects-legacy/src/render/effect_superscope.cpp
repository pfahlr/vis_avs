#include "avs/effects/render/effect_superscope.h"

#include <algorithm>
#include <cmath>

namespace avs::effects::render {

SuperScopeEffect::SuperScopeEffect() {
  // Default scripts (simple spiral pattern)
  initScript_ = "n=800";
  frameScript_ = "t=t-0.05";
  beatScript_ = "";
  pointScript_ = "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d";
}

void SuperScopeEffect::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("init")) {
    initScript_ = params.getString("init", initScript_);
  }
  if (params.contains("frame")) {
    frameScript_ = params.getString("frame", frameScript_);
  }
  if (params.contains("beat")) {
    beatScript_ = params.getString("beat", beatScript_);
  }
  if (params.contains("point")) {
    pointScript_ = params.getString("point", pointScript_);
  }
  if (params.contains("draw_mode")) {
    drawMode_ = params.getInt("draw_mode", drawMode_);
  }
  if (params.contains("audio_channel")) {
    audioChannel_ = params.getInt("audio_channel", audioChannel_);
  }

  // TODO: Mark scripts for recompilation
}

bool SuperScopeEffect::render(avs::core::RenderContext& context) {
  // TODO: Full implementation requires:
  // 1. Initialize EEL VM if not done
  // 2. Compile scripts if needed
  // 3. Bind variables to VM
  // 4. Execute init script (once)
  // 5. Execute frame script
  // 6. Execute beat script (if beat)
  // 7. For each point:
  //    - Get audio value from context.audioSpectrum or waveform
  //    - Set i and v variables
  //    - Execute point script
  //    - Read x, y, red, green, blue, skip
  //    - Render point or line
  //
  // For now, render a placeholder pattern to show the effect is active

  // Simple placeholder: draw a circle of points
  w_ = static_cast<double>(context.width);
  h_ = static_cast<double>(context.height);
  const int numPoints = 100;

  for (int a = 0; a < numPoints; ++a) {
    const double angle = (a / static_cast<double>(numPoints)) * 2.0 * M_PI;
    const double radius = 0.5;

    // Normalized coordinates [-1, 1]
    x_ = std::cos(angle) * radius;
    y_ = std::sin(angle) * radius;

    // Convert to screen coordinates
    const int sx = static_cast<int>((x_ + 1.0) * w_ * 0.5);
    const int sy = static_cast<int>((y_ + 1.0) * h_ * 0.5);

    if (sx >= 0 && sx < context.width && sy >= 0 && sy < context.height) {
      const std::size_t offset = (sy * context.width + sx) * 4;
      if (offset + 3 < context.framebuffer.size) {
        context.framebuffer.data[offset + 0] = 255;  // R
        context.framebuffer.data[offset + 1] = 255;  // G
        context.framebuffer.data[offset + 2] = 255;  // B
        context.framebuffer.data[offset + 3] = 255;  // A
      }
    }
  }

  return true;
}

std::uint32_t SuperScopeEffect::getCurrentColor() {
  if (colors_.empty()) {
    return 0xFFFFFF;
  }

  // Cycle through color palette with smooth interpolation
  const int colorIndex = colorPos_ / 64;
  const int blend = colorPos_ % 64;

  const std::uint32_t c1 = colors_[colorIndex % colors_.size()];
  const std::uint32_t c2 = colors_[(colorIndex + 1) % colors_.size()];

  const int r1 = (c1 >> 16) & 0xFF;
  const int g1 = (c1 >> 8) & 0xFF;
  const int b1 = c1 & 0xFF;

  const int r2 = (c2 >> 16) & 0xFF;
  const int g2 = (c2 >> 8) & 0xFF;
  const int b2 = c2 & 0xFF;

  const int r = (r1 * (63 - blend) + r2 * blend) / 64;
  const int g = (g1 * (63 - blend) + g2 * blend) / 64;
  const int b = (b1 * (63 - blend) + b2 * blend) / 64;

  return (r << 16) | (g << 8) | b;
}

void SuperScopeEffect::advanceColorCycle() {
  colorPos_++;
  if (colorPos_ >= static_cast<int>(colors_.size()) * 64) {
    colorPos_ = 0;
  }
}

void SuperScopeEffect::renderDot(avs::core::RenderContext& context, int x, int y,
                                  std::uint32_t color) {
  if (x < 0 || x >= context.width || y < 0 || y >= context.height) {
    return;
  }

  const std::size_t offset = (y * context.width + x) * 4;
  if (offset + 3 < context.framebuffer.size) {
    context.framebuffer.data[offset + 0] = (color >> 16) & 0xFF;  // R
    context.framebuffer.data[offset + 1] = (color >> 8) & 0xFF;   // G
    context.framebuffer.data[offset + 2] = color & 0xFF;          // B
    context.framebuffer.data[offset + 3] = 255;                   // A
  }
}

void SuperScopeEffect::renderLine(avs::core::RenderContext& context, int x1, int y1,
                                   int x2, int y2, std::uint32_t color, int thickness) {
  // TODO: Implement Bresenham's line algorithm with thickness
  (void)thickness;  // Unused for now
  // For now, just draw the endpoints
  renderDot(context, x1, y1, color);
  renderDot(context, x2, y2, color);
}

}  // namespace avs::effects::render
