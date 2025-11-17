#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include <avs/core.hpp>

namespace avs::effects::geometry {

void clear(FrameBufferView& fb, const ColorRGBA8& color);
void copyFrom(FrameBufferView& dst, const FrameBufferView& src);

void blendPixel(FrameBufferView& fb, int x, int y, const ColorRGBA8& color,
                std::uint8_t coverage = 255);

void drawHorizontalSpan(FrameBufferView& fb, int x0, int x1, int y, const ColorRGBA8& color);
void drawThickLine(FrameBufferView& fb, int x0, int y0, int x1, int y1, int thickness,
                   const ColorRGBA8& color);
void fillRectangle(FrameBufferView& fb, int x, int y, int w, int h, const ColorRGBA8& color);
void strokeRectangle(FrameBufferView& fb, int x, int y, int w, int h, int thickness,
                     const ColorRGBA8& color);
void drawCircle(FrameBufferView& fb, int cx, int cy, int radius, const ColorRGBA8& color,
                bool filled, int thickness);
void fillTriangle(FrameBufferView& fb, const Vec2i& p0, const Vec2i& p1, const Vec2i& p2,
                  const ColorRGBA8& color);
void strokeTriangle(FrameBufferView& fb, const Vec2i& p0, const Vec2i& p1, const Vec2i& p2,
                    int thickness, const ColorRGBA8& color);
void fillPolygon(FrameBufferView& fb, const std::vector<Vec2i>& points, const ColorRGBA8& color);
void strokePolygon(FrameBufferView& fb, const std::vector<Vec2i>& points, int thickness,
                   const ColorRGBA8& color);

ColorRGBA8 withAlpha(ColorRGBA8 color, int alpha);
ColorRGBA8 makeColor(std::uint32_t rgb, int alpha);

std::vector<Vec2i> parsePointList(std::string_view text);

}  // namespace avs::effects::geometry
