#include <avs/effects/filters/effect_conv3x3.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>

#include <avs/effects/filters/filter_common.h>

namespace avs::effects::filters {

Convolution3x3::Convolution3x3() {
  kernel_.fill(0.0f);
  kernel_[4] = 1.0f;
}

void Convolution3x3::parseKernel(std::string_view kernelText) {
  if (kernelText.empty()) {
    return;
  }
  std::array<float, 9> parsed = kernel_;
  std::stringstream ss{std::string(kernelText)};
  float value = 0.0f;
  std::size_t index = 0;
  while (ss >> value && index < parsed.size()) {
    parsed[index++] = value;
  }
  if (index == 0) {
    return;
  }
  for (; index < parsed.size(); ++index) {
    parsed[index] = 0.0f;
  }
  kernel_ = parsed;
}

void Convolution3x3::setParams(const avs::core::ParamBlock& params) {
  parseKernel(params.getString("kernel", params.getString("matrix", "")));
  if (params.contains("divisor")) {
    divisor_ = params.getFloat("divisor", divisor_);
  } else {
    const float sum = std::accumulate(kernel_.begin(), kernel_.end(), 0.0f);
    divisor_ = std::abs(sum) > 1e-6f ? sum : 1.0f;
  }
  if (std::abs(divisor_) < 1e-6f) {
    divisor_ = 1.0f;
  }
  bias_ = params.getFloat("bias", bias_);
  clampOutput_ = params.getBool("clamp", clampOutput_);
  preserveAlpha_ = params.getBool("preserve_alpha", preserveAlpha_);
}

bool Convolution3x3::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }

  const int width = context.width;
  const int height = context.height;
  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  ensureScratch(scratch_, totalBytes);
  std::copy(pixels, pixels + totalBytes, scratch_.begin());

  const float divisor = std::abs(divisor_) < 1e-6f ? 1.0f : divisor_;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float accum[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      for (int ky = -1; ky <= 1; ++ky) {
        const int sy = clampIndex(y + ky, 0, height - 1);
        for (int kx = -1; kx <= 1; ++kx) {
          const int sx = clampIndex(x + kx, 0, width - 1);
          const float weight = kernel_[(ky + 1) * 3 + (kx + 1)];
          const std::uint8_t* srcPx = scratch_.data() + (static_cast<std::size_t>(sy) * width + static_cast<std::size_t>(sx)) * 4u;
          accum[0] += weight * static_cast<float>(srcPx[0]);
          accum[1] += weight * static_cast<float>(srcPx[1]);
          accum[2] += weight * static_cast<float>(srcPx[2]);
          if (!preserveAlpha_) {
            accum[3] += weight * static_cast<float>(srcPx[3]);
          }
        }
      }
      std::uint8_t* dstPx = pixels + (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u;
      for (int channel = 0; channel < 3; ++channel) {
        float value = accum[channel] / divisor + bias_;
        if (clampOutput_) {
          value = std::clamp(value, 0.0f, 255.0f);
        }
        const int rounded = static_cast<int>(std::round(value));
        if (clampOutput_) {
          dstPx[channel] = clampByte(rounded);
        } else {
          dstPx[channel] = static_cast<std::uint8_t>(rounded);
        }
      }
      if (preserveAlpha_) {
        dstPx[3] = scratch_[(static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4u + 3];
      } else {
        float alphaValue = accum[3] / divisor + bias_;
        if (clampOutput_) {
          alphaValue = std::clamp(alphaValue, 0.0f, 255.0f);
        }
        const int roundedAlpha = static_cast<int>(std::round(alphaValue));
        if (clampOutput_) {
          dstPx[3] = clampByte(roundedAlpha);
        } else {
          dstPx[3] = static_cast<std::uint8_t>(roundedAlpha);
        }
      }
    }
  }

  return true;
}

}  // namespace avs::effects::filters

