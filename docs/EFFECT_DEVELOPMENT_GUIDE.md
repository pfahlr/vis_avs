# Effect Development Guide

This guide teaches you how to implement new visual effects for vis_avs. Whether you're porting an existing effect or creating something original, this document will walk you through the process.

## Table of Contents

1. [Effect System Overview](#effect-system-overview)
2. [Effect Lifecycle](#effect-lifecycle)
3. [Anatomy of an Effect](#anatomy-of-an-effect)
4. [Step-by-Step Tutorial](#step-by-step-tutorial)
5. [Parameter System](#parameter-system)
6. [Multi-threaded Effects](#multi-threaded-effects)
7. [Testing Your Effect](#testing-your-effect)
8. [Best Practices](#best-practices)
9. [Common Patterns](#common-patterns)
10. [Examples](#examples)

## Effect System Overview

### The IEffect Interface

All effects implement the `IEffect` interface:

```cpp
class IEffect {
public:
  virtual ~IEffect() = default;

  // Single-threaded rendering (required)
  virtual bool render(RenderContext& context) = 0;

  // Multi-threaded rendering (optional)
  virtual bool smp_render(RenderContext& context,
                          int threadId,
                          int maxThreads) {
    return render(context);  // Default: fall back to single-threaded
  }

  // Parameter configuration (required)
  virtual void setParams(const ParamBlock& params) = 0;

  // Multi-threading capability (optional)
  virtual bool supportsMultiThreaded() const { return false; }

  // Serialization (optional)
  virtual ParamBlock getParams() const { return {}; }
};
```

### Effect Categories

Effects are organized into categories:

| Category | Purpose | Examples |
|----------|---------|----------|
| **render** | Generate new pixels | Superscope, Starfield, Text |
| **trans** | Transform existing pixels | Blur, Movement, Mirror |
| **misc** | Utilities & control | Comment, Buffer Save |
| **prime** | Modern rewrites | Zoom, Blend, Clear |

### The RenderContext

Effects receive a `RenderContext` containing:

```cpp
struct RenderContext {
  // Frame state
  uint64_t frameIndex;        // Current frame number
  float deltaSeconds;         // Time since last frame

  // Display dimensions
  int width;
  int height;

  // Pixel data (RGBA, 32-bit per pixel)
  PixelBufferView framebuffer;  // width * height * 4 bytes

  // Audio data
  AudioBufferView audioSpectrum;  // FFT spectrum
  bool audioBeat;                  // Beat detected this frame
  float audioRMS;                  // RMS amplitude

  // Random number generation
  DeterministicRng rng;  // Seeded RNG for reproducibility
};
```

## Effect Lifecycle

1. **Registration** - Effect factory registered with `EffectRegistry`
2. **Creation** - Factory creates effect instance
3. **Configuration** - `setParams()` called with initial parameters
4. **Rendering Loop**:
   - `render()` or `smp_render()` called each frame
   - Effect reads from/writes to `context.framebuffer`
5. **Destruction** - RAII cleanup when effect removed

```
┌─────────────┐
│ Registration│ (static initialization)
└──────┬──────┘
       │
       v
┌─────────────┐
│  Creation   │ (factory pattern)
└──────┬──────┘
       │
       v
┌─────────────┐
│setParams()  │ (initial config)
└──────┬──────┘
       │
       v
    ┌──┴──┐
    │LOOP │
    └──┬──┘
       │
       v
┌─────────────┐
│  render()   │ (every frame)
└──────┬──────┘
       │
       v
┌─────────────┐
│ Destruction │ (RAII)
└─────────────┘
```

## Anatomy of an Effect

Let's examine a simple effect - a horizontal mirror:

### Header File (`effect_mirror.h`)

```cpp
#pragma once
#include "avs/core/IEffect.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs {
namespace effects {

class MirrorEffect : public core::IEffect {
public:
  MirrorEffect() = default;
  ~MirrorEffect() override = default;

  bool render(core::RenderContext& context) override;
  void setParams(const core::ParamBlock& params) override;
  core::ParamBlock getParams() const override;

private:
  enum class Mode {
    Horizontal,
    Vertical,
    Both
  };

  Mode mode_ = Mode::Horizontal;
};

}  // namespace effects
}  // namespace avs
```

### Implementation File (`effect_mirror.cpp`)

```cpp
#include "avs/effects/trans/effect_mirror.h"
#include <cstring>
#include <algorithm>

namespace avs {
namespace effects {

bool MirrorEffect::render(core::RenderContext& context) {
  const int width = context.width;
  const int height = context.height;
  uint32_t* pixels = reinterpret_cast<uint32_t*>(context.framebuffer.data());

  if (mode_ == Mode::Horizontal || mode_ == Mode::Both) {
    // Mirror left half to right half
    for (int y = 0; y < height; ++y) {
      uint32_t* row = pixels + y * width;
      const int halfWidth = width / 2;

      for (int x = 0; x < halfWidth; ++x) {
        row[width - 1 - x] = row[x];
      }
    }
  }

  if (mode_ == Mode::Vertical || mode_ == Mode::Both) {
    // Mirror top half to bottom half
    const int halfHeight = height / 2;
    const size_t rowBytes = width * sizeof(uint32_t);

    for (int y = 0; y < halfHeight; ++y) {
      uint32_t* topRow = pixels + y * width;
      uint32_t* bottomRow = pixels + (height - 1 - y) * width;
      std::memcpy(bottomRow, topRow, rowBytes);
    }
  }

  return true;
}

void MirrorEffect::setParams(const core::ParamBlock& params) {
  int modeInt = params.getInt("mode", 0);
  mode_ = static_cast<Mode>(modeInt);
}

core::ParamBlock MirrorEffect::getParams() const {
  core::ParamBlock params;
  params.setInt("mode", static_cast<int>(mode_));
  return params;
}

// Register the effect with the factory system
AVS_LEGACY_REGISTER_EFFECT("Trans / Mirror", MirrorEffect);

}  // namespace effects
}  // namespace avs
```

### Key Components

1. **Header** - Class declaration, public interface
2. **Implementation** - Rendering logic, parameter handling
3. **Registration** - `AVS_LEGACY_REGISTER_EFFECT` macro

## Step-by-Step Tutorial

Let's create a new effect called "Color Pulse" that pulses colors based on audio beats.

### Step 1: Create Header File

Create `libs/avs-effects-legacy/include/avs/effects/trans/effect_color_pulse.h`:

```cpp
#pragma once
#include "avs/core/IEffect.hpp"
#include "avs/core/RenderContext.hpp"
#include "avs/core/ParamBlock.hpp"

namespace avs {
namespace effects {

class ColorPulseEffect : public core::IEffect {
public:
  ColorPulseEffect() = default;
  ~ColorPulseEffect() override = default;

  bool render(core::RenderContext& context) override;
  void setParams(const core::ParamBlock& params) override;
  core::ParamBlock getParams() const override;

private:
  // Parameters
  uint32_t pulseColor_ = 0xFFFF0000;  // Red
  float intensity_ = 0.5f;
  float decayRate_ = 0.95f;

  // State
  float currentPulse_ = 0.0f;
};

}  // namespace effects
}  // namespace avs
```

### Step 2: Implement Effect Logic

Create `libs/avs-effects-legacy/src/trans/effect_color_pulse.cpp`:

```cpp
#include "avs/effects/trans/effect_color_pulse.h"
#include <algorithm>

namespace avs {
namespace effects {

bool ColorPulseEffect::render(core::RenderContext& context) {
  // Update pulse on beat
  if (context.audioBeat) {
    currentPulse_ = intensity_;
  } else {
    currentPulse_ *= decayRate_;
  }

  // Early exit if pulse is negligible
  if (currentPulse_ < 0.01f) {
    return true;
  }

  // Extract RGB components from pulse color
  const uint8_t pr = (pulseColor_ >> 16) & 0xFF;
  const uint8_t pg = (pulseColor_ >> 8) & 0xFF;
  const uint8_t pb = pulseColor_ & 0xFF;

  // Apply pulse to all pixels
  const int totalPixels = context.width * context.height;
  uint32_t* pixels = reinterpret_cast<uint32_t*>(context.framebuffer.data());

  for (int i = 0; i < totalPixels; ++i) {
    uint32_t pixel = pixels[i];

    // Extract RGB
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8) & 0xFF;
    uint8_t b = pixel & 0xFF;

    // Blend with pulse color
    r = std::min(255, static_cast<int>(r + pr * currentPulse_));
    g = std::min(255, static_cast<int>(g + pg * currentPulse_));
    b = std::min(255, static_cast<int>(b + pb * currentPulse_));

    // Write back
    pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
  }

  return true;
}

void ColorPulseEffect::setParams(const core::ParamBlock& params) {
  pulseColor_ = params.getInt("color", 0xFFFF0000);
  intensity_ = params.getFloat("intensity", 0.5f);
  decayRate_ = params.getFloat("decay", 0.95f);
}

core::ParamBlock ColorPulseEffect::getParams() const {
  core::ParamBlock params;
  params.setInt("color", pulseColor_);
  params.setFloat("intensity", intensity_);
  params.setFloat("decay", decayRate_);
  return params;
}

// Register with factory
AVS_LEGACY_REGISTER_EFFECT("Trans / Color Pulse", ColorPulseEffect);

}  // namespace effects
}  // namespace avs
```

### Step 3: Add to Build System

Edit `libs/avs-effects-legacy/CMakeLists.txt`:

```cmake
# Find the trans sources section
set(TRANS_SOURCES
  src/trans/effect_blur.cpp
  src/trans/effect_mirror.cpp
  # ... other effects ...
  src/trans/effect_color_pulse.cpp  # Add this line
)
```

### Step 4: Build and Test

```bash
cd build
cmake --build . --target avs-effects-legacy
./apps/avs-player/avs-player --preset test.avs
```

### Step 5: Write Tests

Create `tests/core/test_color_pulse.cpp`:

```cpp
#include <gtest/gtest.h>
#include "avs/effects/trans/effect_color_pulse.h"
#include "avs/core/RenderContext.hpp"

TEST(ColorPulseEffect, PulsesOnBeat) {
  avs::effects::ColorPulseEffect effect;

  // Setup
  avs::core::ParamBlock params;
  params.setInt("color", 0xFFFF0000);  // Red
  params.setFloat("intensity", 1.0f);
  effect.setParams(params);

  // Create render context
  const int width = 320;
  const int height = 240;
  std::vector<uint8_t> pixels(width * height * 4, 0);

  avs::core::RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.frameIndex = 0;
  ctx.audioBeat = true;  // Trigger beat

  // Render
  ASSERT_TRUE(effect.render(ctx));

  // Verify pixels changed (should have red added)
  uint32_t* pixelData = reinterpret_cast<uint32_t*>(pixels.data());
  EXPECT_NE(pixelData[0], 0x00000000);
}

TEST(ColorPulseEffect, DecaysOverTime) {
  avs::effects::ColorPulseEffect effect;

  avs::core::ParamBlock params;
  params.setFloat("decay", 0.5f);
  effect.setParams(params);

  // Setup context
  const int width = 10;
  const int height = 10;
  std::vector<uint8_t> pixels(width * height * 4, 0xFF);

  avs::core::RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.audioBeat = false;  // No beat

  // Capture initial state after beat
  ctx.audioBeat = true;
  effect.render(ctx);
  uint32_t firstFrame = reinterpret_cast<uint32_t*>(pixels.data())[0];

  // Render without beat
  ctx.audioBeat = false;
  effect.render(ctx);
  uint32_t secondFrame = reinterpret_cast<uint32_t*>(pixels.data())[0];

  // Pulse should decay (second frame closer to original)
  EXPECT_NE(firstFrame, secondFrame);
}
```

Add to `tests/CMakeLists.txt`:

```cmake
add_executable(test_color_pulse
  core/test_color_pulse.cpp
)
target_link_libraries(test_color_pulse
  avs-effects-legacy
  GTest::gtest_main
)
add_test(NAME ColorPulse COMMAND test_color_pulse)
```

## Parameter System

### ParamBlock API

```cpp
class ParamBlock {
public:
  // Setters
  void setInt(const std::string& key, int value);
  void setFloat(const std::string& key, float value);
  void setString(const std::string& key, const std::string& value);
  void setBool(const std::string& key, bool value);

  // Getters (with defaults)
  int getInt(const std::string& key, int defaultValue = 0) const;
  float getFloat(const std::string& key, float defaultValue = 0.0f) const;
  std::string getString(const std::string& key, const std::string& defaultValue = "") const;
  bool getBool(const std::string& key, bool defaultValue = false) const;
};
```

### Parameter Best Practices

1. **Always provide defaults** - Makes effects robust
2. **Validate ranges** - Clamp or reject invalid values
3. **Document parameters** - Comment what each does
4. **Use semantic names** - `blur_amount` not `val1`

### Example: Validated Parameters

```cpp
void BlurEffect::setParams(const core::ParamBlock& params) {
  // Get with default
  int radius = params.getInt("radius", 1);

  // Validate and clamp
  radius = std::clamp(radius, 1, 100);

  // Store
  blurRadius_ = radius;

  // Enum parameters
  int modeInt = params.getInt("mode", 0);
  if (modeInt >= 0 && modeInt < static_cast<int>(BlurMode::COUNT)) {
    mode_ = static_cast<BlurMode>(modeInt);
  }
}
```

## Multi-threaded Effects

Multi-threading can significantly improve performance for pixel-heavy effects.

### Enabling Multi-threading

```cpp
class BlurEffect : public core::IEffect {
public:
  // Indicate support
  bool supportsMultiThreaded() const override { return true; }

  // Implement multi-threaded rendering
  bool smp_render(core::RenderContext& context,
                  int threadId,
                  int maxThreads) override;

  // Single-threaded fallback
  bool render(core::RenderContext& context) override {
    return smp_render(context, 0, 1);
  }

private:
  // Thread-safe state
  std::vector<uint8_t> tempBuffer_;
};
```

### Work Distribution Pattern

```cpp
bool BlurEffect::smp_render(core::RenderContext& context,
                             int threadId,
                             int maxThreads) {
  const int height = context.height;
  const int width = context.width;

  // Thread 0 prepares shared resources
  if (threadId == 0) {
    tempBuffer_.resize(width * height * 4);
  }

  // Implicit barrier - all threads wait here

  // Divide work by rows
  const int rowsPerThread = height / maxThreads;
  const int startRow = threadId * rowsPerThread;
  const int endRow = (threadId == maxThreads - 1)
                       ? height
                       : startRow + rowsPerThread;

  // Each thread processes its rows
  for (int y = startRow; y < endRow; ++y) {
    processRow(y, context, tempBuffer_);
  }

  return true;
}
```

### Thread Safety Rules

1. **Read-only context** - Never modify `RenderContext` structure
2. **No shared writes** - Each thread writes to distinct memory
3. **Thread-local temporaries** - Use thread-local storage for scratch data
4. **Atomic operations** - Use atomics for shared counters

### Thread-local Storage Example

```cpp
bool BlurEffect::smp_render(core::RenderContext& context,
                             int threadId,
                             int maxThreads) {
  // Thread-local prefix sum array
  thread_local std::vector<int> prefixSum;

  if (prefixSum.size() < context.width) {
    prefixSum.resize(context.width);
  }

  // Use prefixSum without contention
  // ...
}
```

## Testing Your Effect

### Unit Tests

Test individual effect behavior:

```cpp
TEST(MyEffect, RendersCorrectly) {
  MyEffect effect;

  // Configure
  avs::core::ParamBlock params;
  params.setInt("param1", 42);
  effect.setParams(params);

  // Setup context
  const int width = 100;
  const int height = 100;
  std::vector<uint8_t> pixels(width * height * 4, 0);

  avs::core::RenderContext ctx;
  ctx.width = width;
  ctx.height = height;
  ctx.framebuffer = {pixels.data(), pixels.size()};
  ctx.frameIndex = 0;

  // Render
  ASSERT_TRUE(effect.render(ctx));

  // Verify results
  // ...
}
```

### Integration Tests

Test with real presets:

```bash
# Create test preset
cat > test_my_effect.avs << EOF
Clear Screen
My Effect: param1=42, param2=0.5
EOF

# Test rendering
./apps/avs-player/avs-player --preset test_my_effect.avs --frames 10
```

### Golden Hash Tests

Detect rendering regressions:

```bash
# Generate golden hash
./tools/gen_golden_md5 --preset test_my_effect.avs \
  --frames 10 --seed 1234 > expected.md5.json

# Validate in CI
ctest -R my_effect_golden
```

## Best Practices

### Performance

1. **Minimize allocations** - Reuse buffers, avoid `new` in render loop
2. **Cache-friendly access** - Access pixels row-by-row (spatial locality)
3. **SIMD when possible** - Use vectorized operations for bulk work
4. **Profile first** - Don't optimize prematurely

### Code Quality

1. **RAII** - Use smart pointers, vectors (not raw `new`/`delete`)
2. **Const correctness** - Mark read-only data as `const`
3. **Early returns** - Exit fast for no-op cases
4. **Comments** - Explain non-obvious algorithms

### Maintainability

1. **Small functions** - Keep `render()` under 100 lines
2. **Helper methods** - Extract complex logic to private methods
3. **Named constants** - No magic numbers
4. **Consistent style** - Follow clang-format

## Common Patterns

### Pattern 1: Blend Mode

```cpp
uint32_t blend(uint32_t src, uint32_t dst, BlendMode mode) {
  const uint8_t sr = (src >> 16) & 0xFF;
  const uint8_t sg = (src >> 8) & 0xFF;
  const uint8_t sb = src & 0xFF;

  const uint8_t dr = (dst >> 16) & 0xFF;
  const uint8_t dg = (dst >> 8) & 0xFF;
  const uint8_t db = dst & 0xFF;

  uint8_t r, g, b;

  switch (mode) {
    case BlendMode::Add:
      r = std::min(255, sr + dr);
      g = std::min(255, sg + dg);
      b = std::min(255, sb + db);
      break;

    case BlendMode::Multiply:
      r = (sr * dr) / 255;
      g = (sg * dg) / 255;
      b = (sb * db) / 255;
      break;

    // ... other modes
  }

  return 0xFF000000 | (r << 16) | (g << 8) | b;
}
```

### Pattern 2: Double Buffering

```cpp
bool BlurEffect::render(core::RenderContext& context) {
  const int totalBytes = context.width * context.height * 4;

  // Allocate temp buffer (cached across frames)
  if (tempBuffer_.size() != totalBytes) {
    tempBuffer_.resize(totalBytes);
  }

  // Copy to temp
  std::memcpy(tempBuffer_.data(), context.framebuffer.data(), totalBytes);

  // Read from temp, write to framebuffer
  applyBlur(tempBuffer_.data(), context.framebuffer.data(),
            context.width, context.height);

  return true;
}
```

### Pattern 3: Audio Reactivity

```cpp
bool ReactiveEffect::render(core::RenderContext& context) {
  // Beat detection
  if (context.audioBeat) {
    beatIntensity_ = 1.0f;
  } else {
    beatIntensity_ *= 0.95f;  // Decay
  }

  // Spectrum analysis
  const float bass = context.audioSpectrum[0];  // Low frequencies
  const float treble = context.audioSpectrum[15];  // High frequencies

  // Scale effect by audio
  const float scale = 1.0f + bass * 0.5f;

  // Apply effect
  // ...
}
```

### Pattern 4: Deterministic Randomness

```cpp
bool RandomEffect::render(core::RenderContext& context) {
  // Use context.rng for reproducible randomness
  const int x = context.rng.next() % context.width;
  const int y = context.rng.next() % context.height;

  // DON'T use std::rand() - not deterministic!

  drawPixel(x, y, context.framebuffer);
  return true;
}
```

## Examples

### Example 1: Simple Color Tint

```cpp
bool TintEffect::render(core::RenderContext& context) {
  const int totalPixels = context.width * context.height;
  uint32_t* pixels = reinterpret_cast<uint32_t*>(context.framebuffer.data());

  const uint8_t tr = (tintColor_ >> 16) & 0xFF;
  const uint8_t tg = (tintColor_ >> 8) & 0xFF;
  const uint8_t tb = tintColor_ & 0xFF;

  for (int i = 0; i < totalPixels; ++i) {
    uint32_t pixel = pixels[i];

    uint8_t r = ((pixel >> 16) & 0xFF) * tr / 255;
    uint8_t g = ((pixel >> 8) & 0xFF) * tg / 255;
    uint8_t b = (pixel & 0xFF) * tb / 255;

    pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
  }

  return true;
}
```

### Example 2: Fade to Black

```cpp
bool FadeEffect::render(core::RenderContext& context) {
  const int totalPixels = context.width * context.height;
  uint32_t* pixels = reinterpret_cast<uint32_t*>(context.framebuffer.data());

  const int fadeAmount = 5;  // Fade by 5/256 per frame

  for (int i = 0; i < totalPixels; ++i) {
    uint32_t pixel = pixels[i];

    int r = (pixel >> 16) & 0xFF;
    int g = (pixel >> 8) & 0xFF;
    int b = pixel & 0xFF;

    r = std::max(0, r - fadeAmount);
    g = std::max(0, g - fadeAmount);
    b = std::max(0, b - fadeAmount);

    pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
  }

  return true;
}
```

### Example 3: Pixelation (Mosaic)

```cpp
bool MosaicEffect::render(core::RenderContext& context) {
  if (blockSize_ <= 1) return true;  // No-op

  const int width = context.width;
  const int height = context.height;
  uint32_t* pixels = reinterpret_cast<uint32_t*>(context.framebuffer.data());

  for (int by = 0; by < height; by += blockSize_) {
    for (int bx = 0; bx < width; bx += blockSize_) {
      // Sample top-left pixel of block
      uint32_t blockColor = pixels[by * width + bx];

      // Fill entire block
      for (int y = by; y < std::min(by + blockSize_, height); ++y) {
        for (int x = bx; x < std::min(bx + blockSize_, width); ++x) {
          pixels[y * width + x] = blockColor;
        }
      }
    }
  }

  return true;
}
```

## Next Steps

- Study existing effects in `libs/avs-effects-legacy/src/`
- Read [CODEBASE_ARCHITECTURE.md](CODEBASE_ARCHITECTURE.md)
- Check `codex/jobs/` for effect implementation tasks
- Join discussions on GitHub Issues

Happy effect development! 🎨✨
