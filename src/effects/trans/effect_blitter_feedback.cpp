#include "effects/trans/effect_blitter_feedback.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace avs::effects::trans {

namespace {

std::string toLowerCopy(std::string_view value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return result;
}

std::vector<std::string> tokenizeMode(std::string_view value) {
  std::vector<std::string> tokens;
  std::string current;
  for (char ch : value) {
    if (std::isspace(static_cast<unsigned char>(ch)) || ch == '+' || ch == '|' || ch == ',' || ch == '/') {
      if (!current.empty()) {
        tokens.push_back(toLowerCopy(current));
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    tokens.push_back(toLowerCopy(current));
  }
  if (tokens.empty()) {
    const std::string lowered = toLowerCopy(value);
    if (!lowered.empty()) {
      tokens.push_back(lowered);
    }
  }
  return tokens;
}

}  // namespace

int BlitterFeedback::normalizeQuadrants(int value) {
  int normalized = value % 4;
  if (normalized < 0) {
    normalized += 4;
  }
  return normalized;
}

void BlitterFeedback::applyModeString(const std::string& mode,
                                      bool& mirrorX,
                                      bool& mirrorY,
                                      int& rotateQuadrants) {
  const auto tokens = tokenizeMode(mode);
  if (tokens.empty()) {
    return;
  }

  bool recognized = false;
  bool rotationSet = false;
  bool newMirrorX = false;
  bool newMirrorY = false;
  int newRotate = 0;

  for (const std::string& token : tokens) {
    if (token.empty()) {
      continue;
    }
    if (token == "none" || token == "identity" || token == "reset") {
      recognized = true;
      rotationSet = true;
      newMirrorX = false;
      newMirrorY = false;
      newRotate = 0;
      continue;
    }
    if (token == "mirrorx" || token == "mirror_x" || token == "flipx" || token == "horizontal") {
      recognized = true;
      newMirrorX = true;
      continue;
    }
    if (token == "mirrory" || token == "mirror_y" || token == "flipy" || token == "vertical") {
      recognized = true;
      newMirrorY = true;
      continue;
    }
    if (token == "mirrorxy" || token == "mirror_both" || token == "flipboth" || token == "flip" ||
        token == "both") {
      recognized = true;
      newMirrorX = true;
      newMirrorY = true;
      continue;
    }
    if (token == "rotate90" || token == "rotate_cw" || token == "cw" || token == "cw90" || token == "90") {
      recognized = true;
      rotationSet = true;
      newRotate = 1;
      continue;
    }
    if (token == "rotate180" || token == "180" || token == "halfturn") {
      recognized = true;
      rotationSet = true;
      newRotate = 2;
      continue;
    }
    if (token == "rotate270" || token == "rotate_ccw" || token == "ccw" || token == "ccw90" || token == "270") {
      recognized = true;
      rotationSet = true;
      newRotate = 3;
      continue;
    }
  }

  if (!recognized) {
    return;
  }

  mirrorX = newMirrorX;
  mirrorY = newMirrorY;
  if (rotationSet) {
    rotateQuadrants = newRotate;
  } else {
    rotateQuadrants = 0;
  }
}

void BlitterFeedback::setParams(const avs::core::ParamBlock& params) {
  bool mirrorX = mirrorX_;
  bool mirrorY = mirrorY_;
  int rotate = rotateQuadrants_;
  bool wrap = wrap_;
  float feedback = feedback_;

  if (params.contains("mode")) {
    applyModeString(params.getString("mode", {}), mirrorX, mirrorY, rotate);
  }

  if (params.contains("mirror_x")) {
    mirrorX = params.getBool("mirror_x", mirrorX);
  }
  if (params.contains("flip_horizontal")) {
    mirrorX = params.getBool("flip_horizontal", mirrorX);
  }
  if (params.contains("mirror_y")) {
    mirrorY = params.getBool("mirror_y", mirrorY);
  }
  if (params.contains("flip_vertical")) {
    mirrorY = params.getBool("flip_vertical", mirrorY);
  }
  if (params.contains("wrap")) {
    wrap = params.getBool("wrap", wrap);
  }

  if (params.contains("rotate_quadrants")) {
    rotate = normalizeQuadrants(params.getInt("rotate_quadrants", rotate));
  }
  if (params.contains("rotate_steps")) {
    rotate = normalizeQuadrants(params.getInt("rotate_steps", rotate));
  }
  if (params.contains("rotate")) {
    const float degrees = params.getFloat("rotate", static_cast<float>(rotate) * 90.0f);
    rotate = normalizeQuadrants(static_cast<int>(std::lround(degrees / 90.0f)));
  }
  if (params.contains("rotate_degrees")) {
    const float degrees = params.getFloat("rotate_degrees", static_cast<float>(rotate) * 90.0f);
    rotate = normalizeQuadrants(static_cast<int>(std::lround(degrees / 90.0f)));
  }

  if (params.contains("feedback")) {
    feedback = std::clamp(params.getFloat("feedback", feedback), 0.0f, 1.0f);
  }
  if (params.contains("gain")) {
    feedback = std::clamp(params.getFloat("gain", feedback), 0.0f, 1.0f);
  }
  if (params.contains("attenuation")) {
    feedback = std::clamp(params.getFloat("attenuation", feedback), 0.0f, 1.0f);
  }

  mirrorX_ = mirrorX;
  mirrorY_ = mirrorY;
  rotateQuadrants_ = normalizeQuadrants(rotate);
  wrap_ = wrap;
  feedback_ = std::clamp(feedback, 0.0f, 1.0f);
}

std::uint8_t BlitterFeedback::applyFeedback(std::uint8_t value) const {
  if (feedback_ >= 0.999f) {
    return value;
  }
  const float scaled = static_cast<float>(value) * feedback_;
  const float clamped = std::clamp(scaled, 0.0f, 255.0f);
  return static_cast<std::uint8_t>(std::lround(clamped));
}

bool BlitterFeedback::render(avs::core::RenderContext& context) {
  if (!prepareHistory(context)) {
    return true;
  }

  const int width = historyWidth();
  const int height = historyHeight();
  if (width <= 0 || height <= 0) {
    return true;
  }

  const std::size_t expectedSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (!context.framebuffer.data || context.framebuffer.size < expectedSize) {
    return true;
  }

  for (int py = 0; py < height; ++py) {
    for (int px = 0; px < width; ++px) {
      const float normX = (static_cast<float>(px) + 0.5f) / static_cast<float>(width);
      const float normY = (static_cast<float>(py) + 0.5f) / static_cast<float>(height);
      float x = normX * 2.0f - 1.0f;
      float y = 1.0f - normY * 2.0f;

      if (mirrorX_) {
        x = -x;
      }
      if (mirrorY_) {
        y = -y;
      }

      float sampleX = x;
      float sampleY = y;
      switch (rotateQuadrants_) {
        case 1: {
          const float tmp = sampleX;
          sampleX = sampleY;
          sampleY = -tmp;
          break;
        }
        case 2:
          sampleX = -sampleX;
          sampleY = -sampleY;
          break;
        case 3: {
          const float tmp = sampleX;
          sampleX = -sampleY;
          sampleY = tmp;
          break;
        }
        default:
          break;
      }

      const auto color = sampleHistory(sampleX, sampleY, wrap_);
      const std::size_t index = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width) +
                                 static_cast<std::size_t>(px)) * 4u;
      context.framebuffer.data[index + 0] = applyFeedback(color[0]);
      context.framebuffer.data[index + 1] = applyFeedback(color[1]);
      context.framebuffer.data[index + 2] = applyFeedback(color[2]);
      context.framebuffer.data[index + 3] = color[3];
    }
  }

  storeHistory(context);
  return true;
}

}  // namespace avs::effects::trans

