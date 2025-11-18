# APE Effect Discovery Guide

## Overview

This document describes the comprehensive APE (AVS Plugin Effect) discovery and fallback system implemented in vis_avs. The system ensures maximum compatibility with presets while prioritizing performance and native implementations.

## Discovery Priority System

When an APE effect is encountered in a preset, the system attempts to load it in this priority order:

### Priority 1: Native Reimplementation (Highest Priority)

The system first checks if a native C++17 reimplementation exists in the effect registry.

**Advantages:**
- Best performance (no emulation overhead)
- Cross-platform compatibility (Linux, macOS, Windows)
- No external dependencies

**Currently Implemented Native APE Effects:**
- `Trans / Channel Shift` - RGB channel swapping
- `Trans / Color Reduction` - Bit depth reduction
- `Trans / Multiplier` - Pixel value multiplication
- `Trans / Multi Delay` - Frame delay buffer
- `Trans / Video Delay` - Video delay buffer

**Implementation Location:** `libs/avs-effects-legacy/src/trans/effect_*.cpp`

### Priority 2: APE DLL in Preset Directory Tree

If no native implementation exists, the system searches for the APE DLL file in the preset's directory tree.

**Search Strategy:**
1. **Preset directory** and all parent directories up to root
2. **Common subdirectories:** `APE/`, `ape/`, `plugins/`, `Plugins/`
3. **Case-insensitive matching** for cross-platform compatibility

**Supported File Extensions:**
- `.dll` - Windows DLL
- `.so` - Linux shared object
- `.ape` - APE plugin file

**Search Example:**
```
/home/user/presets/MyCollection/awesome.avs

Searches in order:
1. /home/user/presets/MyCollection/
2. /home/user/presets/MyCollection/APE/
3. /home/user/presets/MyCollection/ape/
4. /home/user/presets/MyCollection/plugins/
5. /home/user/presets/MyCollection/Plugins/
6. /home/user/presets/
7. /home/user/presets/APE/
... (continues up to root)
8. ~/.config/vis_avs/ape_plugins/
9. /usr/local/share/vis_avs/ape_plugins/
10. /usr/share/vis_avs/ape_plugins/
```

**How It Works:**
- Uses Wine-based APE DLL emulator on Linux
- Native Windows DLL loading on Windows
- DLL kept loaded for effect's lifetime (prevents crashes)
- APEinfo callbacks properly initialized

**Implementation:** `libs/avs-compat/src/ape_loader.cpp:findAPEDLL()`

### Priority 3: Skip Effect (Fallback)

If neither a native implementation nor a DLL is found, the effect is replaced with an "Unknown" placeholder.

**Behavior:**
- Effect is logged as unknown
- Preset continues loading (doesn't fail)
- Visual placeholder shows effect name
- Effect data preserved for round-trip compatibility

**Future Enhancement:** Preset fallback system (try next preset in directory if too many unknown effects)

## APE DLL Emulation Details

### Architecture

The Wine-based APE loader provides Windows API emulation for running Windows .ape DLLs on Linux:

```
┌─────────────────────────────────────┐
│     WineAPEEffect (avs::Effect)     │
│  - Wrapper for APE integration      │
└─────────────────┬───────────────────┘
                  │
        ┌─────────▼──────────┐
        │   WineAPELoader    │
        │ - DLL lifetime mgmt│
        │ - Symbol resolution│
        └─────────┬──────────┘
                  │
     ┌────────────▼─────────────┐
     │ Wine PE Loader (Linux)   │
     │ LoadLibrary/dlopen       │
     └────────────┬─────────────┘
                  │
          ┌───────▼────────┐
          │  APE DLL Code  │
          │  (Windows PE)  │
          └────────────────┘
```

### Critical Bug Fixes (Already Implemented)

1. **DLL Lifetime Management:** DLL handle transferred to effect, freed only after effect destruction
2. **APEinfo Callbacks:** `_AVS_APE_SetExtInfo` called before instance creation to provide:
   - Global registers (100 doubles)
   - EEL VM interface
   - gmegabuf access
   - Line blend mode

Without these fixes, APE effects would crash immediately!

## Adding New Native APE Implementations

### Step 1: Study Original Implementation

Research the effect using these resources:
- https://github.com/grandchild/vis_avs/tree/main/avs/vis_avs
- https://github.com/Const-me/vis_avs_dx
- https://avs.sh/collections/APEs/
- https://github.com/grandchild/webvs/tree/master

### Step 2: Create Effect Class

**Header:** `libs/avs-effects-legacy/include/avs/effects/*/effect_<name>.h`
```cpp
#pragma once
#include <avs/core/IEffect.hpp>

namespace avs::effects::<category> {

class <EffectName> : public avs::core::IEffect {
 public:
  <EffectName>();
  ~<EffectName>() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

 private:
  // Effect-specific members
};

}  // namespace avs::effects::<category>
```

**Implementation:** `libs/avs-effects-legacy/src/*/effect_<name>.cpp`

### Step 3: Register Effect

In `libs/avs-effects-legacy/src/effect_registry.cpp`:
```cpp
#include <avs/effects/<category>/effect_<name>.h>

namespace {
std::unique_ptr<Effect> make<EffectName>(const LegacyEffectEntry& entry, ParsedPreset& result) {
  auto effect = std::make_unique<<EffectName>>();
  // Load binary configuration from entry.payload if needed
  return effect;
}
}  // namespace

namespace avs::effects::legacy {
void registerLegacyEffects() {
  AVS_LEGACY_REGISTER_EFFECT("Trans / <Name>", make<EffectName>);
  // ...
}
}  // namespace
```

### Step 4: Add to CMakeLists.txt

File is auto-discovered via GLOB, just ensure it matches pattern in:
`libs/avs-effects-legacy/CMakeLists.txt`

## Known APE Effects to Implement

Based on the URLs provided and common presets:

### High Priority (Common in Presets)
- **Texer II (Acko.net)** - Texture mapping effect
- **ColorMap** - Color palette mapping
- **Normalise** - Audio normalization
- **FramerateLimiter** - FPS control

### Medium Priority
- **ConvolutionFilter (Holden03)** - Custom kernel convolution
- **AVSTrans (TomyLobo)** - Advanced transitions
- **PictureII** - Image overlay
- **AddBorders** - Frame border rendering

### Lower Priority
- **Triangle** - Triangle rendering
- **GlobalVariables** - Shared state between effects
- **MultiFilter** - Multiple filter composition
- **Texer** - Legacy texture mapping

## Testing Your Implementation

### Unit Tests
```cpp
// tests/effects/test_<effect>.cpp
#include <gtest/gtest.h>
#include <avs/effects/<category>/effect_<name>.h>

TEST(<EffectName>Test, BasicRender) {
  avs::effects::<category>::<EffectName> effect;
  // Test rendering logic
}
```

### Integration Tests
1. Create test preset in `tests/data/`
2. Run: `./build/avs-player --preset tests/data/your_test.avs --headless --wav tests/data/test.wav --frames 60`
3. Verify output hashes match expected

### Visual Validation
```bash
./build/avs-player --preset path/to/test.avs
# Press 'r' to reload preset
# Visual inspection of effect output
```

## Preset Fallback System (Planned)

When a preset cannot be fully loaded (too many unknown effects), the system should:

1. **Count Unknown Effects:** Track how many effects fell back to placeholder
2. **Set Threshold:** If >50% of effects are unknown, mark preset as unusable
3. **Try Next Preset:** Search directory for alternative `.avs` files
4. **Recurse Directory Tree:** Continue until valid preset found or directory exhausted

**Implementation Location:** `apps/avs-player/main.cpp` (pending)

## Configuration

### Global APE Search Paths

Default paths (can be customized via `setAPESearchPaths()`):
- `~/.config/vis_avs/ape_plugins/`
- `/usr/local/share/vis_avs/ape_plugins/`
- `/usr/share/vis_avs/ape_plugins/`

### Environment Variables

```bash
# Skip Wine APE loader (use only native effects)
export AVS_SKIP_APE_WINE=1

# Add custom APE search path
export AVS_APE_PATH=/path/to/ape/plugins
```

## Debugging

### Enable Verbose Logging
```bash
# APE loader will log search paths and loading attempts
./build/avs-player --preset test.avs 2>&1 | grep -E 'INFO|WARNING'
```

### Common Issues

**Problem:** APE DLL not found
**Solution:** Check search paths, ensure DLL is in preset directory or global path

**Problem:** APE loads but crashes
**Solution:** Verify APEinfo callbacks are initialized (should already be fixed)

**Problem:** Effect shows as "Unknown"
**Solution:** Check if native implementation exists or DLL is accessible

## References

- [AVS Original Source](https://github.com/grandchild/vis_avs)
- [AVS DX Port](https://github.com/Const-me/vis_avs_dx)
- [AVS Blog](https://avs.sh/blog/)
- [APE Collection](https://avs.sh/collections/APEs/)
- [AVSDB Plugin Database](https://www.avsdb.net/Files/dir/1)
- [WebVS JavaScript Port](https://github.com/grandchild/webvs)

## See Also

- [APE Architecture Notes](./avs_original_source_port_notes/ape-plugin-architecture.md)
- [APE Emulator Usage](./ape_emulator_usage.md)
- [APE Linux Support Strategy](./ape_linux_support_strategy.md)
- [Effect Development Guide](./EFFECT_DEVELOPMENT_GUIDE.md)
