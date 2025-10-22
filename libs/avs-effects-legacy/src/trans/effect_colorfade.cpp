#include <avs/effects/legacy/trans/effect_colorfade.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>

namespace avs::effects::trans {

namespace {

constexpr std::array<const char*, 3> kOffsetKeysA{"offset_a", "offset0", "offset_r"};
constexpr std::array<const char*, 3> kOffsetKeysB{"offset_b", "offset1", "offset_g"};
constexpr std::array<const char*, 3> kOffsetKeysC{"offset_c", "offset2", "offset_b"};
constexpr std::array<const char*, 3> kBeatKeysA{"beat_offset_a", "beat_offset0", "beat_offset_r"};
constexpr std::array<const char*, 3> kBeatKeysB{"beat_offset_b", "beat_offset1", "beat_offset_g"};
constexpr std::array<const char*, 3> kBeatKeysC{"beat_offset_c", "beat_offset2", "beat_offset_b"};

int saturatingAdd(int base, int delta) {
  const int value = base + delta;
  return value <= 0 ? 0 : (value >= 255 ? 255 : value);
}

int readOffset(const avs::core::ParamBlock& params,
               const std::array<const char*, 3>& keys,
               int fallback) {
  for (const char* key : keys) {
    if (params.contains(key)) {
      return params.getInt(key, fallback);
    }
  }
  return fallback;
}

}  // namespace

Colorfade::Colorfade() {
  baseOffsets_ = {8, -8, -8};
  beatOffsets_ = baseOffsets_;
  currentOffsets_ = baseOffsets_;
}

int Colorfade::clampOffset(int value) {
  return std::clamp(value, kMinOffset, kMaxOffset);
}

void Colorfade::setParams(const avs::core::ParamBlock& params) {
  if (params.contains("flags")) {
    const int flags = params.getInt("flags", 0);
    enabled_ = (flags & 1) != 0;
    randomizeOnBeat_ = (flags & 2) != 0;
    smooth_ = (flags & 4) != 0;
  } else {
    enabled_ = params.getBool("enabled", enabled_);
    randomizeOnBeat_ = params.getBool("randomize_on_beat", randomizeOnBeat_);
    randomizeOnBeat_ = params.getBool("randomize", randomizeOnBeat_);
    smooth_ = params.getBool("smooth", smooth_);
    smooth_ = params.getBool("use_beat_faders", smooth_);
  }

  baseOffsets_[0] = clampOffset(readOffset(params, kOffsetKeysA, baseOffsets_[0]));
  baseOffsets_[1] = clampOffset(readOffset(params, kOffsetKeysB, baseOffsets_[1]));
  baseOffsets_[2] = clampOffset(readOffset(params, kOffsetKeysC, baseOffsets_[2]));

  beatOffsets_[0] = clampOffset(readOffset(params, kBeatKeysA, beatOffsets_[0]));
  beatOffsets_[1] = clampOffset(readOffset(params, kBeatKeysB, beatOffsets_[1]));
  beatOffsets_[2] = clampOffset(readOffset(params, kBeatKeysC, beatOffsets_[2]));

  if (!smooth_) {
    currentOffsets_ = baseOffsets_;
  } else {
    for (int& value : currentOffsets_) {
      value = clampOffset(value);
    }
  }
}

void Colorfade::updateOffsets(avs::core::RenderContext& context) {
  if (!smooth_) {
    currentOffsets_ = baseOffsets_;
    return;
  }

  auto stepTowards = [](int& value, int target) {
    if (value < target) {
      ++value;
    } else if (value > target) {
      --value;
    }
  };

  stepTowards(currentOffsets_[0], baseOffsets_[0]);
  stepTowards(currentOffsets_[1], baseOffsets_[1]);
  stepTowards(currentOffsets_[2], baseOffsets_[2]);

  if (context.audioBeat) {
    if (randomizeOnBeat_) {
      const auto nextRange = [&context](std::uint32_t range) {
        std::uint32_t value = context.rng.nextUint32();
        return static_cast<int>(value % range);
      };
      int offsetA = nextRange(32u) - 6;
      int offsetB = nextRange(64u) - 32;
      if (offsetB < 0 && offsetB > -16) {
        offsetB = -32;
      }
      if (offsetB >= 0 && offsetB < 16) {
        offsetB = 32;
      }
      int offsetC = nextRange(32u) - 6;
      currentOffsets_[0] = clampOffset(offsetA);
      currentOffsets_[1] = clampOffset(offsetB);
      currentOffsets_[2] = clampOffset(offsetC);
    } else {
      currentOffsets_ = beatOffsets_;
    }
  }
}

bool Colorfade::render(avs::core::RenderContext& context) {
  if (!enabled_ || context.framebuffer.data == nullptr || context.width <= 0 || context.height <= 0) {
    return true;
  }

  updateOffsets(context);

  const bool hasEffect = (currentOffsets_[0] != 0 || currentOffsets_[1] != 0 || currentOffsets_[2] != 0);
  if (!hasEffect) {
    return true;
  }

  const std::array<std::array<int, 3>, 4> tables{
      std::array<int, 3>{currentOffsets_[2], currentOffsets_[1], currentOffsets_[0]},
      std::array<int, 3>{currentOffsets_[1], currentOffsets_[0], currentOffsets_[2]},
      std::array<int, 3>{currentOffsets_[0], currentOffsets_[2], currentOffsets_[1]},
      std::array<int, 3>{currentOffsets_[2], currentOffsets_[2], currentOffsets_[2]},
  };

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);

  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    int tableIndex = 3;
    if (px[1] > px[2] && px[1] > px[0]) {
      tableIndex = 0;
    } else if (px[0] > px[1] && px[0] > px[2]) {
      tableIndex = 1;
    } else if (px[2] > px[0] && px[2] > px[1]) {
      tableIndex = 2;
    }

    px[0] = static_cast<std::uint8_t>(saturatingAdd(px[0], tables[tableIndex][0]));
    px[1] = static_cast<std::uint8_t>(saturatingAdd(px[1], tables[tableIndex][1]));
    px[2] = static_cast<std::uint8_t>(saturatingAdd(px[2], tables[tableIndex][2]));
  }

  return true;
}

}  // namespace avs::effects::trans
