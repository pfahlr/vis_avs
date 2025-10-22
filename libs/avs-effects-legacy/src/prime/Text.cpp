#include <avs/effects/Primitives.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

#include <avs/effects/primitives/primitive_common.hpp>
#include "text/text_renderer.h"

namespace avs::effects {

namespace {

std::string toLower(std::string_view text) {
  std::string out(text);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

bool fetchBool(const avs::core::ParamBlock& params, std::string_view key, bool fallback) {
  if (!params.contains(std::string(key))) {
    return fallback;
  }
  if (auto value = params.getBool(std::string(key), fallback); value != fallback || fallback) {
    return value;
  }
  return params.getInt(std::string(key), fallback ? 1 : 0) != 0;
}

}  // namespace

void Text::setParams(const avs::core::ParamBlock& params) {
  text_ = params.getString("text", text_);
  posX_ = params.getInt("x", posX_);
  posY_ = params.getInt("y", posY_);
  size_ = params.getInt("size", params.getInt("height", size_));
  widthOverride_ = params.getInt("glyphwidth", params.getInt("width", widthOverride_));
  spacing_ = params.getInt("spacing", spacing_);
  color_ = params.getInt("color", color_);
  alpha_ = params.getInt("alpha", alpha_);
  outlineColor_ = params.getInt("outlinecolor", outlineColor_);
  outlineAlpha_ = params.getInt("outlinealpha", outlineAlpha_);
  outlineSize_ = std::max(0, params.getInt("outlinesize", outlineSize_));
  shadowColor_ = params.getInt("shadowcolor", shadowColor_);
  shadowAlpha_ = params.getInt("shadowalpha", shadowAlpha_);
  shadowOffsetX_ = params.getInt("shadowoffsetx", params.getInt("shadow_offset_x", shadowOffsetX_));
  shadowOffsetY_ = params.getInt("shadowoffsety", params.getInt("shadow_offset_y", shadowOffsetY_));
  shadowBlur_ = std::max(0, params.getInt("shadowblur", params.getInt("shadow_blur", shadowBlur_)));
  antialias_ = fetchBool(params, "antialias", antialias_);
  shadow_ = fetchBool(params, "shadow", shadow_);

  std::string hal = params.getString("halign", halign_);
  std::string val = params.getString("valign", valign_);
  std::string combined = params.getString("align", "");
  if (!combined.empty()) {
    std::string lower = toLower(combined);
    if (lower == "center" || lower == "middle") {
      hal = "center";
      val = "middle";
    } else if (lower == "left" || lower == "right") {
      hal = lower;
    } else if (lower == "top" || lower == "bottom") {
      val = lower;
    }
  }
  halign_ = toLower(hal);
  valign_ = toLower(val);
}

bool Text::render(avs::core::RenderContext& context) {
  if (!context.framebuffer.data || context.width <= 0 || context.height <= 0) {
    return true;
  }
  if (text_.empty()) {
    return true;
  }

  text::TextRenderer renderer;
  text::RasterOptions opts;
  opts.pixelHeight = std::max(1, size_);
  opts.pixelWidth = widthOverride_ > 0 ? widthOverride_ : 0;
  opts.spacing = std::max(0, spacing_);
  opts.antialias = antialias_;
  text::Surface surface = renderer.render(text_, opts);
  if (surface.width <= 0 || surface.height <= 0) {
    return true;
  }

  int leftPad = outlineSize_;
  int rightPad = outlineSize_;
  int topPad = outlineSize_;
  int bottomPad = outlineSize_;
  if (shadow_) {
    if (shadowOffsetX_ < 0) {
      leftPad = std::max(leftPad, -shadowOffsetX_ + shadowBlur_);
    } else {
      rightPad = std::max(rightPad, shadowOffsetX_ + shadowBlur_);
    }
    if (shadowOffsetY_ < 0) {
      topPad = std::max(topPad, -shadowOffsetY_ + shadowBlur_);
    } else {
      bottomPad = std::max(bottomPad, shadowOffsetY_ + shadowBlur_);
    }
  }

  const int outWidth = surface.width + leftPad + rightPad;
  const int outHeight = surface.height + topPad + bottomPad;
  std::vector<std::uint8_t> baseMask(static_cast<std::size_t>(outWidth) * outHeight, 0);
  for (int y = 0; y < surface.height; ++y) {
    for (int x = 0; x < surface.width; ++x) {
      baseMask[static_cast<std::size_t>(y + topPad) * outWidth + (x + leftPad)] =
          surface.mask[static_cast<std::size_t>(y) * surface.width + x];
    }
  }

  std::vector<std::uint8_t> outlineMask;
  if (outlineSize_ > 0) {
    outlineMask = detail::createStrokeMask(baseMask, outWidth, outHeight, outlineSize_);
  }

  std::vector<std::uint8_t> shadowMask;
  if (shadow_) {
    shadowMask.assign(baseMask.size(), 0);
    for (int y = 0; y < outHeight; ++y) {
      for (int x = 0; x < outWidth; ++x) {
        const std::uint8_t coverage = baseMask[static_cast<std::size_t>(y) * outWidth + x];
        if (!coverage) {
          continue;
        }
        const int nx = x + shadowOffsetX_;
        const int ny = y + shadowOffsetY_;
        if (nx < 0 || ny < 0 || nx >= outWidth || ny >= outHeight) {
          continue;
        }
        auto& dest = shadowMask[static_cast<std::size_t>(ny) * outWidth + nx];
        dest = std::max(dest, coverage);
      }
    }
    if (shadowBlur_ > 0) {
      detail::boxBlur(shadowMask, outWidth, outHeight, shadowBlur_);
    }
  }

  const detail::RGBA textColor = detail::colorFromInt(color_, detail::clampByte(alpha_));
  const detail::RGBA outlineColor = detail::colorFromInt(outlineColor_, detail::clampByte(outlineAlpha_));
  const detail::RGBA shadowColor = detail::colorFromInt(shadowColor_, detail::clampByte(shadowAlpha_));

  int drawX = posX_;
  int drawY = posY_;
  if (halign_ == "center" || halign_ == "middle") {
    drawX -= outWidth / 2;
  } else if (halign_ == "right") {
    drawX -= outWidth;
  }
  if (valign_ == "middle" || valign_ == "center") {
    drawY -= outHeight / 2;
  } else if (valign_ == "bottom") {
    drawY -= outHeight;
  }
  drawX -= leftPad;
  drawY -= topPad;

  for (int y = 0; y < outHeight; ++y) {
    for (int x = 0; x < outWidth; ++x) {
      const int dstX = drawX + x;
      const int dstY = drawY + y;
      if (dstX < 0 || dstY < 0 || dstX >= context.width || dstY >= context.height) {
        continue;
      }
      const std::size_t idx = static_cast<std::size_t>(y) * outWidth + x;
      if (!shadowMask.empty()) {
        const std::uint8_t coverage = shadowMask[idx];
        if (coverage) {
          detail::blendPixel(context, dstX, dstY, shadowColor, coverage);
        }
      }
      if (!outlineMask.empty()) {
        const std::uint8_t coverage = outlineMask[idx];
        if (coverage) {
          detail::blendPixel(context, dstX, dstY, outlineColor, coverage);
        }
      }
      const std::uint8_t coverage = baseMask[idx];
      if (coverage) {
        detail::blendPixel(context, dstX, dstY, textColor, coverage);
      }
    }
  }

  return true;
}

}  // namespace avs::effects

