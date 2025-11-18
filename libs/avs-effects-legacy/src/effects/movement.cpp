#include "avs/effects.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace avs {

namespace {
constexpr double kPI = 3.14159265358979323846;
}

class MovementEffect : public Effect {
 public:
  MovementEffect(int effect_type, bool blend, bool sourcemapped, bool rectangular,
                 bool subpixel, bool wrap, std::string effect_exp = "")
      : effect_type_(effect_type),
        blend_(blend),
        sourcemapped_(sourcemapped),
        rectangular_(rectangular),
        subpixel_(subpixel),
        wrap_(wrap),
        effect_exp_(std::move(effect_exp)) {}

  void init(int w, int h) override {
    width_ = w;
    height_ = h;
    generateTransformTable();
  }

  void process(const Framebuffer& in, Framebuffer& out) override {
    if (effect_type_ == 0 || transform_table_.empty()) {
      // No effect - just copy
      if (out.rgba.size() != in.rgba.size()) {
        out.rgba.resize(in.rgba.size());
      }
      std::copy(in.rgba.begin(), in.rgba.end(), out.rgba.begin());
      return;
    }

    const int w = width_;
    const int h = height_;
    const size_t pixel_count = static_cast<size_t>(w) * h;

    // Ensure output buffer is sized correctly
    if (out.rgba.size() != pixel_count * 4) {
      out.rgba.resize(pixel_count * 4);
    }

    const std::uint8_t* inp = in.rgba.data();
    std::uint8_t* outp = out.rgba.data();

    // Handle source-mapped mode
    if (sourcemapped_) {
      // In source-mapped mode, we map from input position to output position
      if (!blend_) {
        std::fill(out.rgba.begin(), out.rgba.end(), 0);
      } else {
        std::copy(in.rgba.begin(), in.rgba.end(), out.rgba.begin());
      }

      for (size_t i = 0; i < pixel_count; ++i) {
        int target_offset = transform_table_[i];
        if (target_offset >= 0 && target_offset < static_cast<int>(pixel_count)) {
          size_t target_idx = static_cast<size_t>(target_offset) * 4;
          size_t src_idx = i * 4;

          // Blend MAX (take brightest)
          outp[target_idx + 0] = std::max(outp[target_idx + 0], inp[src_idx + 0]);
          outp[target_idx + 1] = std::max(outp[target_idx + 1], inp[src_idx + 1]);
          outp[target_idx + 2] = std::max(outp[target_idx + 2], inp[src_idx + 2]);
          outp[target_idx + 3] = std::max(outp[target_idx + 3], inp[src_idx + 3]);
        }
      }

      if (blend_) {
        // Average blend with original
        for (size_t i = 0; i < pixel_count * 4; i += 4) {
          outp[i + 0] = (outp[i + 0] + inp[i + 0]) >> 1;
          outp[i + 1] = (outp[i + 1] + inp[i + 1]) >> 1;
          outp[i + 2] = (outp[i + 2] + inp[i + 2]) >> 1;
          outp[i + 3] = (outp[i + 3] + inp[i + 3]) >> 1;
        }
      }
    } else {
      // Normal mode: map from output position to input position
      for (size_t i = 0; i < pixel_count; ++i) {
        int src_offset = transform_table_[i];
        if (src_offset >= 0 && src_offset < static_cast<int>(pixel_count)) {
          size_t src_idx = static_cast<size_t>(src_offset) * 4;
          size_t dst_idx = i * 4;

          outp[dst_idx + 0] = inp[src_idx + 0];
          outp[dst_idx + 1] = inp[src_idx + 1];
          outp[dst_idx + 2] = inp[src_idx + 2];
          outp[dst_idx + 3] = inp[src_idx + 3];
        } else {
          // Out of bounds - set to black
          size_t dst_idx = i * 4;
          outp[dst_idx + 0] = 0;
          outp[dst_idx + 1] = 0;
          outp[dst_idx + 2] = 0;
          outp[dst_idx + 3] = 255;
        }
      }

      if (blend_) {
        // Average blend with input
        for (size_t i = 0; i < pixel_count * 4; i += 4) {
          outp[i + 0] = (outp[i + 0] + inp[i + 0]) >> 1;
          outp[i + 1] = (outp[i + 1] + inp[i + 1]) >> 1;
          outp[i + 2] = (outp[i + 2] + inp[i + 2]) >> 1;
          outp[i + 3] = (outp[i + 3] + inp[i + 3]) >> 1;
        }
      }
    }
  }

 private:
  void generateTransformTable() {
    const int w = width_;
    const int h = height_;
    const size_t pixel_count = static_cast<size_t>(w) * h;

    transform_table_.resize(pixel_count);

    // Effect 1: Slight fuzzify
    if (effect_type_ == 1) {
      for (int i = 0; i < static_cast<int>(pixel_count); ++i) {
        int r = i + (rand() % 3) - 1 + ((rand() % 3) - 1) * w;
        transform_table_[i] = std::min(static_cast<int>(pixel_count) - 1, std::max(r, 0));
      }
      return;
    }

    // Effect 2: Shift rotate left
    if (effect_type_ == 2) {
      for (int y = 0; y < h; ++y) {
        int lp = w / 64;
        for (int x = 0; x < w; ++x) {
          transform_table_[y * w + x] = y * w + lp;
          lp++;
          if (lp >= w) lp -= w;
        }
      }
      return;
    }

    // Effect 7: Blocky partial out
    if (effect_type_ == 7) {
      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          if ((x & 2) || (y & 2)) {
            transform_table_[y * w + x] = y * w + x;
          } else {
            int xp = w / 2 + (((x & ~1) - w / 2) * 7) / 8;
            int yp = h / 2 + (((y & ~1) - h / 2) * 7) / 8;
            transform_table_[y * w + x] = yp * w + xp;
          }
        }
      }
      return;
    }

    // Radial effects (3-17)
    if (effect_type_ >= 3 && effect_type_ <= 17) {
      const double max_d = std::sqrt(static_cast<double>(w * w + h * h) / 4.0);

      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          double xd = x - (w / 2.0);
          double yd = y - (h / 2.0);
          double d = std::sqrt(xd * xd + yd * yd);
          double r = std::atan2(yd, xd);

          int xo = 0, yo = 0;

          // Apply transformation based on effect type
          applyRadialTransform(effect_type_, r, d, max_d, xo, yo);

          // Convert back to cartesian coordinates
          double tmp1 = (h / 2.0) + std::sin(r) * d + 0.5 + (yo * h) * (1.0 / 256.0);
          double tmp2 = (w / 2.0) + std::cos(r) * d + 0.5 + (xo * w) * (1.0 / 256.0);

          int oh = static_cast<int>(tmp1);
          int ow = static_cast<int>(tmp2);

          // Handle wrapping/clamping
          if (wrap_) {
            ow %= w;
            oh %= h;
            if (ow < 0) ow += w;
            if (oh < 0) oh += h;
          } else {
            if (ow < 0) ow = 0;
            if (ow >= w) ow = w - 1;
            if (oh < 0) oh = 0;
            if (oh >= h) oh = h - 1;
          }

          transform_table_[y * w + x] = oh * w + ow;
        }
      }
      return;
    }

    // Default: identity transform
    for (size_t i = 0; i < pixel_count; ++i) {
      transform_table_[i] = static_cast<int>(i);
    }
  }

  void applyRadialTransform(int effect, double& r, double& d, double max_d, int& xo, int& yo) {
    (void)yo;  // Reserved for future use
    const double d_norm = d / max_d;

    switch (effect) {
      case 3:  // Big swirl out
        r += 0.1 - 0.2 * d_norm;
        d *= 0.96;
        break;

      case 4:  // Medium swirl
        d *= 0.99 * (1.0 - std::sin(r) / 32.0);
        r += 0.03 * std::sin(d_norm * kPI * 4);
        break;

      case 5:  // Sunburster
        d *= 0.94 + (std::cos(r * 32.0) * 0.06);
        break;

      case 6:  // Swirl to center
        d *= 1.01 + (std::cos(r * 4.0) * 0.04);
        r += 0.03 * std::sin(d_norm * kPI * 4);
        break;

      case 8:  // Swirling around both ways
        r += 0.1 * std::sin(d_norm * kPI * 5);
        break;

      case 9: {  // Bubbling outward
        double t = std::sin(d_norm * kPI);
        d -= 8 * t * t * t * t * t;
        break;
      }

      case 10: {  // Bubbling outward with swirl
        double t = std::sin(d_norm * kPI);
        d -= 8 * t * t * t * t * t;
        t = std::cos(d_norm * kPI / 2.0);
        r += 0.1 * t * t * t;
        break;
      }

      case 11:  // 5 pointed distro
        d *= 0.95 + (std::cos(r * 5.0 - kPI / 2.50) * 0.03);
        break;

      case 12:  // Tunneling
        r += 0.04;
        d *= 0.96 + std::cos(d_norm * kPI) * 0.05;
        break;

      case 13: {  // Bleedin'
        double t = std::cos(d_norm * kPI);
        r += 0.07 * t;
        d *= 0.98 + t * 0.10;
        break;
      }

      case 14:  // Shifted big swirl out
        r += 0.1 - 0.2 * d_norm;
        d *= 0.96;
        xo = 8;
        break;

      case 15:  // Psychotic beaming outward
        d = max_d * 0.15;
        break;

      case 16:  // Cosine radial 3-way
        r = std::cos(r * 3);
        break;

      case 17:  // Spinny tube
        d *= (1 - ((d_norm - 0.35) * 0.5));
        r += 0.1;
        break;

      default:
        break;
    }
  }

  int effect_type_ = 0;
  bool blend_ = false;
  bool sourcemapped_ = false;
  bool rectangular_ = false;
  bool subpixel_ = false;
  bool wrap_ = false;
  std::string effect_exp_;

  int width_ = 0;
  int height_ = 0;
  std::vector<int> transform_table_;
};

// Factory function for use by the effect registry
std::unique_ptr<Effect> createMovementEffect(int effect, bool blend, bool sourcemapped,
                                              bool rectangular, bool subpixel, bool wrap,
                                              const std::string& effect_exp) {
  return std::make_unique<MovementEffect>(effect, blend, sourcemapped, rectangular, subpixel, wrap,
                                           effect_exp);
}

}  // namespace avs
