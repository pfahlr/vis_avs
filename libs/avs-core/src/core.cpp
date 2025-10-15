#include "avs/core.hpp"
#include <algorithm>
#include <cmath>

namespace avs {

  int getCoreVersion() { return 1; }

  static inline uint8_t u8clamp(int v){ return (uint8_t)std::min(255, std::max(0, v)); }
  static inline uint8_t u8clampf(float v){ return (uint8_t)std::min(255.0f, std::max(0.0f, std::roundf(v))); }

  static inline void wrap_coord(float& c, int max, Wrap w){
    if(w == Wrap::Clamp){
      if(c < 0) c = 0;
      if(c > max-1) c = (float)(max-1);
    } else if(w == Wrap::Wrap){
      while(c < 0) c += max;
      while(c >= max) c -= max;
    } else {
      // Mirror
      float m = std::fmod(std::fabs(c), (float)max);
      int pi = (int)std::floor(std::fabs(c) / max);
      if(pi % 2 == 1) m = (float)max - 1 - m;
      c = m;
    }
  }

  ColorRGBA8 sampleRGBA(const FrameBufferView& src, float x, float y, const SampleOptions& opt){
    // Implement nearest + bilinear with simple wrap handling.
    float fx = x;
    float fy = y;
    wrap_coord(fx, src.width, opt.wrap);
    wrap_coord(fy, src.height, opt.wrap);

    if(opt.filter == Filter::Nearest){
      int ix = (int)std::roundf(fx);
      int iy = (int)std::roundf(fy);
      ix = std::clamp(ix, 0, src.width - 1);
      iy = std::clamp(iy, 0, src.height - 1);
      const uint8_t* p = src.data + iy * src.stride + ix * 4;
      return ColorRGBA8{p[0], p[1], p[2], p[3]};
    }else{
      int x0 = (int)std::floor(fx);
      int y0 = (int)std::floor(fy);
      int x1 = std::min(x0 + 1, src.width - 1);
      int y1 = std::min(y0 + 1, src.height - 1);
      float tx = fx - x0;
      float ty = fy - y0;
      auto px = [&](int xi, int yi){
        const uint8_t* p = src.data + yi * src.stride + xi * 4;
        return ColorRGBA8{p[0], p[1], p[2], p[3]};
      };
      ColorRGBA8 c00 = px(x0,y0), c10 = px(x1,y0), c01 = px(x0,y1), c11 = px(x1,y1);
      auto lerpf = [&](float a, float b, float t){ return a + (b - a) * t; };
      auto mixc = [&](const ColorRGBA8& a, const ColorRGBA8& b, float t){
        return ColorRGBA8{
          u8clampf(lerpf(a.r, b.r, t)),
          u8clampf(lerpf(a.g, b.g, t)),
          u8clampf(lerpf(a.b, b.b, t)),
          u8clampf(lerpf(a.a, b.a, t))
        };
      };
      ColorRGBA8 c0 = mixc(c00, c10, tx);
      ColorRGBA8 c1 = mixc(c01, c11, tx);
      return mixc(c0, c1, ty);
    }
  }

  void blendPixel(ColorRGBA8& dst, const ColorRGBA8& src, BlendMode mode){
    switch(mode){
      case BlendMode::Replace:
        dst = src; break;
      case BlendMode::Add:
        dst.r = u8clamp(dst.r + src.r);
        dst.g = u8clamp(dst.g + src.g);
        dst.b = u8clamp(dst.b + src.b);
        break;
      case BlendMode::Subtract:
        dst.r = u8clamp(dst.r - src.r);
        dst.g = u8clamp(dst.g - src.g);
        dst.b = u8clamp(dst.b - src.b);
        break;
      case BlendMode::Multiply:
        dst.r = u8clampf((dst.r/255.0f)*(src.r/255.0f)*255.0f);
        dst.g = u8clampf((dst.g/255.0f)*(src.g/255.0f)*255.0f);
        dst.b = u8clampf((dst.b/255.0f)*(src.b/255.0f)*255.0f);
        break;
      case BlendMode::Max:
        dst.r = std::max(dst.r, src.r);
        dst.g = std::max(dst.g, src.g);
        dst.b = std::max(dst.b, src.b);
        break;
      case BlendMode::Min:
        dst.r = std::min(dst.r, src.r);
        dst.g = std::min(dst.g, src.g);
        dst.b = std::min(dst.b, src.b);
        break;
      case BlendMode::Average:
        dst.r = (uint8_t)((dst.r + src.r) / 2);
        dst.g = (uint8_t)((dst.g + src.g) / 2);
        dst.b = (uint8_t)((dst.b + src.b) / 2);
        break;
      case BlendMode::Xor:
        dst.r = dst.r ^ src.r;
        dst.g = dst.g ^ src.g;
        dst.b = dst.b ^ src.b;
        break;
      case BlendMode::Fifty:
        dst.r = (uint8_t)((dst.r * 1 + src.r * 1) / 2);
        dst.g = (uint8_t)((dst.g * 1 + src.g * 1) / 2);
        dst.b = (uint8_t)((dst.b * 1 + src.b * 1) / 2);
        break;
    }
  }

} // namespace avs
