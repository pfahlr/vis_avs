#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <sstream>
#include <variant>

#include "avs/effects_render.hpp"
#include "effects/geometry/raster.hpp"
#include "effects/geometry/superscope.hpp"
#include "effects/geometry/text_renderer.hpp"

namespace avs {

void effects::geometry::SuperscopeRuntimeDeleter::operator()(
    effects::geometry::SuperscopeRuntime* ptr) const noexcept {
  delete ptr;
}

namespace {

constexpr int kSuperscopeMaxPoints = 128 * 1024;
constexpr double kPi = 3.14159265358979323846;

int asInt(const ParamValue& value, int fallback) {
  if (const auto* v = std::get_if<int>(&value)) return *v;
  if (const auto* f = std::get_if<float>(&value)) return static_cast<int>(*f);
  if (const auto* b = std::get_if<bool>(&value)) return *b ? 1 : 0;
  return fallback;
}

float asFloat(const ParamValue& value, float fallback) {
  if (const auto* f = std::get_if<float>(&value)) return *f;
  if (const auto* i = std::get_if<int>(&value)) return static_cast<float>(*i);
  if (const auto* b = std::get_if<bool>(&value)) return *b ? 1.0f : 0.0f;
  return fallback;
}

bool asBool(const ParamValue& value, bool fallback) {
  if (const auto* b = std::get_if<bool>(&value)) return *b;
  if (const auto* i = std::get_if<int>(&value)) return *i != 0;
  if (const auto* f = std::get_if<float>(&value)) return std::abs(*f) > 1e-6f;
  if (const auto* s = std::get_if<std::string>(&value)) {
    std::string lower;
    lower.reserve(s->size());
    for (char c : *s)
      lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (lower == "true" || lower == "on" || lower == "yes") return true;
    if (lower == "false" || lower == "off" || lower == "no") return false;
  }
  return fallback;
}

std::string asString(const ParamValue& value, std::string fallback) {
  if (const auto* s = std::get_if<std::string>(&value)) return *s;
  return fallback;
}

ColorRGBA8 asColor(const ParamValue& value, ColorRGBA8 fallback) {
  if (const auto* c = std::get_if<ColorRGBA8>(&value)) return *c;
  if (const auto* i = std::get_if<int>(&value))
    return effects::geometry::makeColor(static_cast<std::uint32_t>(*i), fallback.a);
  if (const auto* s = std::get_if<std::string>(&value)) {
    std::uint32_t rgb = 0;
    std::stringstream ss(*s);
    if (s->rfind("0x", 0) == 0 || s->rfind("0X", 0) == 0) {
      ss >> std::hex >> rgb;
    } else {
      ss >> rgb;
    }
    if (!ss.fail()) {
      return effects::geometry::makeColor(rgb, fallback.a);
    }
  }
  return fallback;
}

ColorRGBA8 withAlpha(ColorRGBA8 color, const ParamValue& alphaValue) {
  if (const auto* i = std::get_if<int>(&alphaValue)) {
    return effects::geometry::withAlpha(color, *i);
  }
  if (const auto* f = std::get_if<float>(&alphaValue)) {
    return effects::geometry::withAlpha(color, static_cast<int>(*f));
  }
  return color;
}

std::vector<Vec2i> parseTrianglePoints(const ParamValue& value) {
  std::string list;
  if (const auto* s = std::get_if<std::string>(&value)) {
    list = *s;
  }
  return effects::geometry::parsePointList(list);
}

std::string toLower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

}  // namespace

SuperscopeEffect::~SuperscopeEffect() = default;

// ---------------- Text Effect ----------------

std::vector<Param> TextEffect::parameters() const {
  return {
      Param{"text", ParamKind::String, settings_.text},
      Param{"x", ParamKind::Int, settings_.x},
      Param{"y", ParamKind::Int, settings_.y},
      Param{"size", ParamKind::Int, settings_.size, 1, 512},
      Param{"width", ParamKind::Int, settings_.glyphWidth, 0, 512},
      Param{"spacing", ParamKind::Int, settings_.spacing, 0, 32},
      Param{"color", ParamKind::Color, settings_.color},
      Param{"outlinecolor", ParamKind::Color, settings_.outline},
      Param{"outlinesize", ParamKind::Int, settings_.outlineSize, 0, 32},
      Param{"shadow", ParamKind::Bool, settings_.shadow},
      Param{"shadowcolor", ParamKind::Color, settings_.shadowColor},
      Param{"shadowoffsetx", ParamKind::Int, settings_.shadowOffsetX, -64, 64},
      Param{"shadowoffsety", ParamKind::Int, settings_.shadowOffsetY, -64, 64},
      Param{"shadowblur", ParamKind::Int, settings_.shadowBlur, 0, 32},
      Param{"antialias", ParamKind::Bool, settings_.antialias},
      Param{"halign",
            ParamKind::Select,
            settings_.halign,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {{"left", "Left"}, {"center", "Center"}, {"right", "Right"}}},
      Param{"valign",
            ParamKind::Select,
            settings_.valign,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {{"top", "Top"}, {"middle", "Middle"}, {"bottom", "Bottom"}}},
  };
}

void TextEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "text") {
    settings_.text = asString(value, settings_.text);
  } else if (name == "x") {
    settings_.x = asInt(value, settings_.x);
  } else if (name == "y") {
    settings_.y = asInt(value, settings_.y);
  } else if (name == "size" || name == "height") {
    settings_.size = std::max(1, asInt(value, settings_.size));
  } else if (name == "width" || name == "glyphwidth") {
    settings_.glyphWidth = std::max(0, asInt(value, settings_.glyphWidth));
  } else if (name == "spacing") {
    settings_.spacing = std::max(0, asInt(value, settings_.spacing));
  } else if (name == "color") {
    settings_.color = asColor(value, settings_.color);
  } else if (name == "alpha") {
    settings_.color = withAlpha(settings_.color, value);
  } else if (name == "outlinecolor") {
    settings_.outline = asColor(value, settings_.outline);
  } else if (name == "outlinealpha") {
    settings_.outline = withAlpha(settings_.outline, value);
  } else if (name == "outlinesize" || name == "outlinewidth") {
    settings_.outlineSize = std::max(0, asInt(value, settings_.outlineSize));
  } else if (name == "shadow") {
    settings_.shadow = asBool(value, settings_.shadow);
  } else if (name == "shadowcolor") {
    settings_.shadowColor = asColor(value, settings_.shadowColor);
  } else if (name == "shadowalpha") {
    settings_.shadowColor = withAlpha(settings_.shadowColor, value);
  } else if (name == "shadowoffsetx") {
    settings_.shadowOffsetX = asInt(value, settings_.shadowOffsetX);
  } else if (name == "shadowoffsety") {
    settings_.shadowOffsetY = asInt(value, settings_.shadowOffsetY);
  } else if (name == "shadowblur") {
    settings_.shadowBlur = std::max(0, asInt(value, settings_.shadowBlur));
  } else if (name == "antialias") {
    settings_.antialias = asBool(value, settings_.antialias);
  } else if (name == "halign") {
    settings_.halign = toLower(asString(value, settings_.halign));
  } else if (name == "valign") {
    settings_.valign = toLower(asString(value, settings_.valign));
  } else if (name == "align") {
    std::string combined = toLower(asString(value, ""));
    if (combined == "center" || combined == "middle") {
      settings_.halign = "center";
      settings_.valign = "middle";
    } else if (combined == "left" || combined == "right") {
      settings_.halign = combined;
    } else if (combined == "top" || combined == "bottom") {
      settings_.valign = combined;
    }
  }
}

namespace {

void dilateMask(std::vector<std::uint8_t>& mask, int width, int height, int radius) {
  if (radius <= 0) return;
  std::vector<std::uint8_t> original = mask;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!original[static_cast<std::size_t>(y) * width + x]) continue;
      for (int oy = -radius; oy <= radius; ++oy) {
        for (int ox = -radius; ox <= radius; ++ox) {
          int nx = x + ox;
          int ny = y + oy;
          if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
          std::size_t idx = static_cast<std::size_t>(ny) * width + nx;
          mask[idx] = std::max(mask[idx], original[static_cast<std::size_t>(y) * width + x]);
        }
      }
    }
  }
}

std::vector<std::uint8_t> createStrokeMask(const std::vector<std::uint8_t>& base, int width,
                                           int height, int radius) {
  if (radius <= 0) return {};
  std::vector<std::uint8_t> mask = base;
  dilateMask(mask, width, height, radius);
  for (std::size_t i = 0; i < mask.size(); ++i) {
    if (base[i] >= mask[i]) mask[i] = 0;
  }
  return mask;
}

void boxBlurMask(std::vector<std::uint8_t>& mask, int width, int height, int radius) {
  if (radius <= 0 || mask.empty()) return;
  const int stride = width + 1;
  std::vector<int> integral(static_cast<std::size_t>(stride) * (height + 1), 0);
  for (int y = 0; y < height; ++y) {
    int rowSum = 0;
    for (int x = 0; x < width; ++x) {
      rowSum += mask[static_cast<std::size_t>(y) * width + x];
      integral[static_cast<std::size_t>(y + 1) * stride + (x + 1)] =
          integral[static_cast<std::size_t>(y) * stride + (x + 1)] + rowSum;
    }
  }
  std::vector<std::uint8_t> output(mask.size(), 0);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int x0 = std::max(0, x - radius);
      const int y0 = std::max(0, y - radius);
      const int x1 = std::min(width, x + radius + 1);
      const int y1 = std::min(height, y + radius + 1);
      const int sum = integral[static_cast<std::size_t>(y1) * stride + x1] -
                      integral[static_cast<std::size_t>(y0) * stride + x1] -
                      integral[static_cast<std::size_t>(y1) * stride + x0] +
                      integral[static_cast<std::size_t>(y0) * stride + x0];
      const int area = (x1 - x0) * (y1 - y0);
      output[static_cast<std::size_t>(y) * width + x] =
          static_cast<std::uint8_t>(sum / std::max(1, area));
    }
  }
  mask.swap(output);
}

}  // namespace

void TextEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  (void)ctx;
  if (settings_.text.empty() || !dst.data) return;
  effects::geometry::text::Renderer renderer;
  effects::geometry::text::RasterOptions opts;
  opts.pixelHeight = settings_.size;
  opts.pixelWidth = settings_.glyphWidth;
  opts.spacing = settings_.spacing;
  opts.antialias = settings_.antialias;
  auto surface = renderer.render(settings_.text, opts);
  if (surface.width <= 0 || surface.height <= 0) return;

  int leftPad = settings_.outlineSize;
  int rightPad = settings_.outlineSize;
  int topPad = settings_.outlineSize;
  int bottomPad = settings_.outlineSize;
  if (settings_.shadow) {
    if (settings_.shadowOffsetX < 0) {
      leftPad = std::max(leftPad, -settings_.shadowOffsetX + settings_.shadowBlur);
    } else {
      rightPad = std::max(rightPad, settings_.shadowOffsetX + settings_.shadowBlur);
    }
    if (settings_.shadowOffsetY < 0) {
      topPad = std::max(topPad, -settings_.shadowOffsetY + settings_.shadowBlur);
    } else {
      bottomPad = std::max(bottomPad, settings_.shadowOffsetY + settings_.shadowBlur);
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
  if (settings_.outlineSize > 0 && settings_.outline.a > 0) {
    outlineMask = createStrokeMask(baseMask, outWidth, outHeight, settings_.outlineSize);
  }

  std::vector<std::uint8_t> shadowMask;
  if (settings_.shadow && settings_.shadowColor.a > 0) {
    shadowMask.assign(baseMask.size(), 0);
    for (int y = 0; y < outHeight; ++y) {
      for (int x = 0; x < outWidth; ++x) {
        std::uint8_t coverage = baseMask[static_cast<std::size_t>(y) * outWidth + x];
        if (!coverage) continue;
        int nx = x + settings_.shadowOffsetX;
        int ny = y + settings_.shadowOffsetY;
        if (nx < 0 || ny < 0 || nx >= outWidth || ny >= outHeight) continue;
        std::size_t idx = static_cast<std::size_t>(ny) * outWidth + nx;
        shadowMask[idx] = std::max(shadowMask[idx], coverage);
      }
    }
    if (settings_.shadowBlur > 0) {
      boxBlurMask(shadowMask, outWidth, outHeight, settings_.shadowBlur);
    }
  }

  int drawX = settings_.x;
  int drawY = settings_.y;
  if (settings_.halign == "center" || settings_.halign == "middle") {
    drawX -= outWidth / 2;
  } else if (settings_.halign == "right") {
    drawX -= outWidth;
  }
  if (settings_.valign == "middle" || settings_.valign == "center") {
    drawY -= outHeight / 2;
  } else if (settings_.valign == "bottom") {
    drawY -= outHeight;
  }
  drawX -= leftPad;
  drawY -= topPad;

  for (int y = 0; y < outHeight; ++y) {
    for (int x = 0; x < outWidth; ++x) {
      int dstX = drawX + x;
      int dstY = drawY + y;
      if (dstX < 0 || dstY < 0 || dstX >= dst.width || dstY >= dst.height) continue;
      std::size_t idx = static_cast<std::size_t>(y) * outWidth + x;
      if (!shadowMask.empty() && shadowMask[idx]) {
        effects::geometry::blendPixel(dst, dstX, dstY, settings_.shadowColor, shadowMask[idx]);
      }
      if (!outlineMask.empty() && outlineMask[idx]) {
        effects::geometry::blendPixel(dst, dstX, dstY, settings_.outline, outlineMask[idx]);
      }
      if (baseMask[idx]) {
        effects::geometry::blendPixel(dst, dstX, dstY, settings_.color, baseMask[idx]);
      }
    }
  }
}

// ---------------- Superscope Effect ----------------

std::vector<Param> SuperscopeEffect::parameters() const {
  return {
      Param{"init", ParamKind::String, initScript_},
      Param{"frame", ParamKind::String, frameScript_},
      Param{"beat", ParamKind::String, beatScript_},
      Param{"point", ParamKind::String, pointScript_},
      Param{"points", ParamKind::Int, overridePoints_.value_or(512), 1, kSuperscopeMaxPoints},
      Param{"linesize", ParamKind::Int, static_cast<int>(overrideThickness_.value_or(1.0f)), 1, 64},
      Param{"drawmode",
            ParamKind::Select,
            overrideLineMode_.value_or(false) ? std::string{"lines"} : std::string{"dots"},
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {{"dots", "Dots"}, {"lines", "Lines"}}},
  };
}

void SuperscopeEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "init") {
    initScript_ = asString(value, initScript_);
  } else if (name == "frame") {
    frameScript_ = asString(value, frameScript_);
  } else if (name == "beat") {
    beatScript_ = asString(value, beatScript_);
  } else if (name == "point" || name == "pixel") {
    pointScript_ = asString(value, pointScript_);
  } else if (name == "points" || name == "n") {
    overridePoints_ =
        std::clamp(asInt(value, overridePoints_.value_or(512)), 1, kSuperscopeMaxPoints);
  } else if (name == "linesize") {
    overrideThickness_ = std::max(1.0f, asFloat(value, overrideThickness_.value_or(1.0f)));
  } else if (name == "drawmode") {
    if (const auto* b = std::get_if<bool>(&value)) {
      overrideLineMode_ = *b;
    } else if (const auto* i = std::get_if<int>(&value)) {
      overrideLineMode_ = *i != 0;
    } else {
      std::string mode =
          toLower(asString(value, overrideLineMode_.value_or(false) ? "lines" : "dots"));
      overrideLineMode_ = (mode == "lines" || mode == "line" || mode == "1");
    }
  }
  if (runtime_) {
    effects::geometry::SuperscopeConfig config{initScript_, frameScript_, beatScript_,
                                               pointScript_};
    runtime_->setScripts(config);
    runtime_->setOverrides(overridePoints_, overrideThickness_, overrideLineMode_);
  }
}

void SuperscopeEffect::init(const InitContext& ctx) {
  runtime_.reset(new effects::geometry::SuperscopeRuntime());
  runtime_->setScripts({initScript_, frameScript_, beatScript_, pointScript_});
  runtime_->setOverrides(overridePoints_, overrideThickness_, overrideLineMode_);
  runtime_->init(ctx);
  initialized_ = true;
}

void SuperscopeEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!runtime_) {
    InitContext initCtx{};
    initCtx.frame_size = FrameSize{dst.width, dst.height};
    initCtx.deterministic = ctx.time.deterministic;
    initCtx.fps_hint = ctx.time.fps_hint;
    init(initCtx);
  }
  runtime_->setOverrides(overridePoints_, overrideThickness_, overrideLineMode_);
  runtime_->update(ctx);
  runtime_->render(ctx, dst);
}

// ---------------- Triangles Effect ----------------

std::vector<Param> TrianglesEffect::parameters() const {
  return {
      Param{"triangles", ParamKind::String, std::string{}},
      Param{"filled", ParamKind::Bool, filled_},
      Param{"color", ParamKind::Color, fillColor_},
      Param{"outlinecolor", ParamKind::Color, outlineColor_},
      Param{"outlinewidth", ParamKind::Int, outlineWidth_, 0, 32},
  };
}

void TrianglesEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "triangles" || name == "points") {
    auto pts = parseTrianglePoints(value);
    triangles_.clear();
    pendingMask_ = {false, false, false};
    for (std::size_t i = 0; i + 2 < pts.size(); i += 3) {
      triangles_.push_back(Triangle{pts[i], pts[i + 1], pts[i + 2]});
    }
  } else if (name == "filled") {
    filled_ = asBool(value, filled_);
  } else if (name == "color") {
    fillColor_ = asColor(value, fillColor_);
  } else if (name == "alpha") {
    fillColor_ = withAlpha(fillColor_, value);
  } else if (name == "outlinecolor") {
    outlineColor_ = asColor(value, outlineColor_);
  } else if (name == "outlinealpha") {
    outlineColor_ = withAlpha(outlineColor_, value);
  } else if (name == "outlinesize" || name == "outlinewidth") {
    outlineWidth_ = std::max(0, asInt(value, outlineWidth_));
  } else if (name == "x1") {
    pendingVertices_[0].x = asInt(value, pendingVertices_[0].x);
    pendingMask_[0] = true;
  } else if (name == "y1") {
    pendingVertices_[0].y = asInt(value, pendingVertices_[0].y);
    pendingMask_[0] = true;
  } else if (name == "x2") {
    pendingVertices_[1].x = asInt(value, pendingVertices_[1].x);
    pendingMask_[1] = true;
  } else if (name == "y2") {
    pendingVertices_[1].y = asInt(value, pendingVertices_[1].y);
    pendingMask_[1] = true;
  } else if (name == "x3") {
    pendingVertices_[2].x = asInt(value, pendingVertices_[2].x);
    pendingMask_[2] = true;
  } else if (name == "y3") {
    pendingVertices_[2].y = asInt(value, pendingVertices_[2].y);
    pendingMask_[2] = true;
  }
  if (pendingMask_[0] && pendingMask_[1] && pendingMask_[2]) {
    triangles_.push_back(Triangle{pendingVertices_[0], pendingVertices_[1], pendingVertices_[2]});
    pendingMask_ = {false, false, false};
  }
}

void TrianglesEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (triangles_.empty()) return;
  for (const auto& tri : triangles_) {
    if (filled_ && fillColor_.a > 0) {
      effects::geometry::fillTriangle(dst, tri.a, tri.b, tri.c, fillColor_);
    }
    if ((!filled_ || outlineWidth_ > 0) && outlineColor_.a > 0) {
      int thickness = std::max(1, outlineWidth_);
      effects::geometry::strokeTriangle(dst, tri.a, tri.b, tri.c, thickness, outlineColor_);
    }
  }
}

// ---------------- Shapes Effect ----------------

std::vector<Param> ShapesEffect::parameters() const {
  return {
      Param{"shape",
            ParamKind::Select,
            std::string{"circle"},
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {{"circle", "Circle"}, {"rect", "Rectangle"}, {"star", "Star"}, {"line", "Line"}}},
      Param{"x", ParamKind::Int, settings_.x},
      Param{"y", ParamKind::Int, settings_.y},
      Param{"radius", ParamKind::Int, settings_.radius, 0, 4096},
      Param{"width", ParamKind::Int, settings_.width, 0, 4096},
      Param{"height", ParamKind::Int, settings_.height, 0, 4096},
      Param{"inner_radius", ParamKind::Int, settings_.innerRadius, 0, 4096},
      Param{"points", ParamKind::Int, settings_.points, 3, 64},
      Param{"rotation", ParamKind::Float, settings_.rotationDeg},
      Param{"filled", ParamKind::Bool, settings_.filled},
      Param{"color", ParamKind::Color, settings_.fillColor},
      Param{"outlinecolor", ParamKind::Color, settings_.outlineColor},
      Param{"outlinewidth", ParamKind::Int, settings_.outlineWidth, 0, 64},
  };
}

void ShapesEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "shape" || name == "type") {
    std::string type = toLower(asString(value, "circle"));
    if (type == "circle")
      settings_.type = ShapeType::Circle;
    else if (type == "rect" || type == "rectangle")
      settings_.type = ShapeType::Rect;
    else if (type == "star")
      settings_.type = ShapeType::Star;
    else if (type == "line")
      settings_.type = ShapeType::Line;
  } else if (name == "x") {
    settings_.x = asInt(value, settings_.x);
  } else if (name == "y") {
    settings_.y = asInt(value, settings_.y);
  } else if (name == "radius") {
    settings_.radius = std::max(0, asInt(value, settings_.radius));
  } else if (name == "width") {
    settings_.width = std::max(0, asInt(value, settings_.width));
  } else if (name == "height") {
    settings_.height = std::max(0, asInt(value, settings_.height));
  } else if (name == "inner_radius") {
    settings_.innerRadius = std::max(0, asInt(value, settings_.innerRadius));
  } else if (name == "points") {
    settings_.points = std::max(3, asInt(value, settings_.points));
  } else if (name == "rotation") {
    settings_.rotationDeg = asFloat(value, settings_.rotationDeg);
  } else if (name == "filled") {
    settings_.filled = asBool(value, settings_.filled);
  } else if (name == "color") {
    settings_.fillColor = asColor(value, settings_.fillColor);
  } else if (name == "alpha") {
    settings_.fillColor = withAlpha(settings_.fillColor, value);
  } else if (name == "outlinecolor") {
    settings_.outlineColor = asColor(value, settings_.outlineColor);
  } else if (name == "outlinealpha") {
    settings_.outlineColor = withAlpha(settings_.outlineColor, value);
  } else if (name == "outlinewidth" || name == "outlinesize") {
    settings_.outlineWidth = std::max(0, asInt(value, settings_.outlineWidth));
  } else if (name == "x2") {
    settings_.lineEnd.x = asInt(value, settings_.lineEnd.x);
    settings_.lineEndSet = true;
  } else if (name == "y2") {
    settings_.lineEnd.y = asInt(value, settings_.lineEnd.y);
    settings_.lineEndSet = true;
  }
}

namespace {

std::vector<Vec2i> makeStarPoints(int cx, int cy, int outerRadius, int innerRadius, int points,
                                  float rotationDeg) {
  std::vector<Vec2i> pts;
  int outer = std::max(0, outerRadius);
  int inner = std::max(0, innerRadius);
  if (inner == 0) inner = outer / 2;
  if (inner <= 0) inner = 1;
  int clampedPoints = std::max(3, points);
  int steps = clampedPoints * 2;
  double angleRad = static_cast<double>(rotationDeg) * kPi / 180.0;
  for (int i = 0; i < steps; ++i) {
    double t = (static_cast<double>(i) / steps) * kPi * 2.0 + angleRad;
    int r = (i % 2 == 0) ? outer : inner;
    int px = cx + static_cast<int>(std::round(r * std::cos(t)));
    int py = cy + static_cast<int>(std::round(r * std::sin(t)));
    pts.push_back(Vec2i{px, py});
  }
  return pts;
}

}  // namespace

void ShapesEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data) return;
  ColorRGBA8 outline = settings_.outlineColor;
  int thickness = std::max(1, settings_.outlineWidth);
  switch (settings_.type) {
    case ShapeType::Circle: {
      if (settings_.filled && settings_.fillColor.a > 0) {
        effects::geometry::drawCircle(dst, settings_.x, settings_.y, settings_.radius,
                                      settings_.fillColor, true, 1);
      }
      if ((!settings_.filled || settings_.outlineWidth > 0) && outline.a > 0) {
        effects::geometry::drawCircle(dst, settings_.x, settings_.y, settings_.radius, outline,
                                      false, thickness);
      }
      break;
    }
    case ShapeType::Rect: {
      int halfW = settings_.width / 2;
      int halfH = settings_.height / 2;
      if (settings_.filled && settings_.fillColor.a > 0) {
        effects::geometry::fillRectangle(dst, settings_.x - halfW, settings_.y - halfH,
                                         settings_.width, settings_.height, settings_.fillColor);
      }
      if ((!settings_.filled || settings_.outlineWidth > 0) && outline.a > 0) {
        effects::geometry::strokeRectangle(dst, settings_.x - halfW, settings_.y - halfH,
                                           settings_.width, settings_.height, thickness, outline);
      }
      break;
    }
    case ShapeType::Star: {
      auto pts = makeStarPoints(settings_.x, settings_.y, settings_.radius, settings_.innerRadius,
                                settings_.points, settings_.rotationDeg);
      if (settings_.filled && settings_.fillColor.a > 0) {
        effects::geometry::fillPolygon(dst, pts, settings_.fillColor);
      }
      if ((!settings_.filled || settings_.outlineWidth > 0) && outline.a > 0) {
        effects::geometry::strokePolygon(dst, pts, thickness, outline);
      }
      break;
    }
    case ShapeType::Line: {
      Vec2i end = settings_.lineEnd;
      if (!settings_.lineEndSet) {
        end = Vec2i{settings_.x + settings_.radius, settings_.y};
      }
      effects::geometry::drawThickLine(dst, settings_.x, settings_.y, end.x, end.y, thickness,
                                       outline.a > 0 ? outline : settings_.fillColor);
      break;
    }
  }
}

// ---------------- Dot Grid Effect ----------------

std::vector<Param> DotGridEffect::parameters() const {
  return {
      Param{"cols", ParamKind::Int, settings_.cols, 1, 512},
      Param{"rows", ParamKind::Int, settings_.rows, 1, 512},
      Param{"spacing_x", ParamKind::Int, settings_.spacingX, 1, 1024},
      Param{"spacing_y", ParamKind::Int, settings_.spacingY, 1, 1024},
      Param{"offset_x", ParamKind::Int, settings_.offsetX, -4096, 4096},
      Param{"offset_y", ParamKind::Int, settings_.offsetY, -4096, 4096},
      Param{"radius", ParamKind::Int, settings_.radius, 0, 1024},
      Param{"color", ParamKind::Color, settings_.colorA},
      Param{"alt_color", ParamKind::Color, settings_.colorB},
      Param{"alternate", ParamKind::Bool, settings_.alternate},
  };
}

void DotGridEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name == "cols") {
    settings_.cols = std::max(1, asInt(value, settings_.cols));
  } else if (name == "rows") {
    settings_.rows = std::max(1, asInt(value, settings_.rows));
  } else if (name == "spacing_x") {
    settings_.spacingX = std::max(1, asInt(value, settings_.spacingX));
  } else if (name == "spacing_y") {
    settings_.spacingY = std::max(1, asInt(value, settings_.spacingY));
  } else if (name == "offset_x") {
    settings_.offsetX = asInt(value, settings_.offsetX);
  } else if (name == "offset_y") {
    settings_.offsetY = asInt(value, settings_.offsetY);
  } else if (name == "radius") {
    settings_.radius = std::max(0, asInt(value, settings_.radius));
  } else if (name == "color") {
    settings_.colorA = asColor(value, settings_.colorA);
  } else if (name == "alt_color") {
    settings_.colorB = asColor(value, settings_.colorB);
  } else if (name == "alternate") {
    settings_.alternate = asBool(value, settings_.alternate);
  }
}

void DotGridEffect::process(const ProcessContext&, FrameBufferView& dst) {
  if (!dst.data) return;
  for (int row = 0; row < settings_.rows; ++row) {
    for (int col = 0; col < settings_.cols; ++col) {
      int x = settings_.offsetX + col * settings_.spacingX;
      int y = settings_.offsetY + row * settings_.spacingY;
      ColorRGBA8 color = settings_.colorA;
      if (settings_.alternate && ((row + col) & 1)) {
        color = settings_.colorB;
      }
      effects::geometry::drawCircle(dst, x, y, settings_.radius, color, true, 1);
    }
  }
}

}  // namespace avs
