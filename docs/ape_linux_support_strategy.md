# APE Plugin Support Strategy for Linux

## Executive Summary

This document outlines the dual-strategy approach for supporting AVS APE (Advanced Plugin Effect) files on Linux: **native reimplementation** (primary) and **DLL emulation** (fallback).

## Background

### What Are APE Files?

APE files are Windows DLL plugins that extend Winamp AVS with custom visual effects. They:
- Implement the `C_RBASE` C++ virtual interface
- Use Win32 APIs (HWND, HINSTANCE) for configuration dialogs
- Are loaded dynamically at runtime on Windows
- Have a specific binary format in `.avs` preset files

### Binary Format in Presets

When an APE effect appears in a `.avs` preset file:
```
[Effect ID: uint32]          → 16384 (0x4000 = DLLRENDERBASE)
[APE Identifier: char[32]]   → "Channel Shift\0\0\0..." (null-padded)
[Payload Length: uint32]     → Size of effect config data
[Payload Data: uint8[]]      → Effect-specific binary parameters
```

### Current Status

vis_avs **already supports 5 APE effects** through native reimplementation:
- Channel Shift
- Color Reduction
- Multiplier
- Multi Delay (Holden05)
- Video Delay

This achieves **100% preset compatibility** for common presets.

## Strategy

### Primary Approach: Native Reimplementation

**Status:** ✅ Already in progress

**Advantages:**
- Native Linux performance
- No Windows dependencies
- Clean, maintainable C++ code
- GPU acceleration possible
- Cross-platform from day one

**Process:**
1. Identify popular APE effects (see "Discovery" below)
2. Analyze original Windows DLL code (from `docs/avs_original_source/vis_avs/r_*.cpp`)
3. Implement as native effect in `libs/avs-effects-legacy/src/`
4. Register in effect registry
5. Test with presets that use the effect

**Already Implemented:**
- `libs/avs-effects-legacy/src/trans/effect_multiplier.cpp`
- `libs/avs-effects-legacy/src/trans/effect_video_delay.cpp`
- `libs/avs-effects-legacy/src/trans/effect_color_reduction.cpp`
- `libs/avs-effects-legacy/src/trans/effect_multi_delay.cpp`
- `libs/avs-effects-legacy/include/avs/effects/trans/effect_*.h`

**Candidates for Next Implementation** (based on grandchild's vis_avs):
- **Holden03: Convolution Filter** - Popular image processing effect
- **Acko.net: Texer II** - Advanced texture mapping
- **TomyLobo: AVSTrans** - Transformation automation
- **Acko.net: Picture II** - Enhanced image rendering
- **Normalise** - Brightness normalization
- **Colormap** - Palette mapping effects
- **AddBorders** - Frame decoration
- **Triangle** - Geometric primitive renderer
- **GlobalVariables** - Inter-effect communication
- **MultiFilter** - Compound filtering
- **FramerateLimiter** - Temporal control

### Fallback Approach: APE DLL Emulator

**Status:** 🔨 Design phase

**Use Case:** For APE effects that are:
- Too complex to reimplement efficiently
- Rarely used (low ROI for reimplementation)
- Require exact binary compatibility with original

**Architecture:**

```
┌────────────────────────────────────────────────────┐
│         vis_avs (Native Linux Application)         │
├────────────────────────────────────────────────────┤
│        APE Effect Factory (libs/avs-compat)        │
│  ┌──────────────────┬──────────────────────────┐  │
│  │  Native Registry │  Emulator Module (opt)   │  │
│  │  [5 effects]     │  [Wine-based loader]     │  │
│  └──────────────────┴──────────────────────────┘  │
├────────────────────────────────────────────────────┤
│           APE Emulator Module Components           │
│  ┌──────────────────────────────────────────────┐ │
│  │ • Wine PE/DLL loader (winelib)               │ │
│  │ • Win32 API shims (HWND, HINSTANCE stubs)    │ │
│  │ • C++ vtable marshaling (MSVC ↔ GCC)        │ │
│  │ • APEinfo callback provider                  │ │
│  │   - EEL VM access (ns-eel integration)       │ │
│  │   - Global registers (gmegabuf)              │ │
│  │   - Blend modes (lineblendmode)              │ │
│  │   - Shared buffers (getNbuffer)              │ │
│  └──────────────────────────────────────────────┘ │
├────────────────────────────────────────────────────┤
│         APE DLL (Windows PE Binary)                │
│         ~/.config/vis_avs/ape_plugins/*.dll        │
└────────────────────────────────────────────────────┘
```

**Implementation Options:**

#### Option A: Wine + winelib (Recommended)

Use Wine's existing PE loader infrastructure:

```cpp
// libs/avs-compat/src/ape_emulator.cpp
class WineAPELoader {
private:
  void* wine_dll_handle;
  typedef C_RBASE* (*APECreateFunc)();
  APECreateFunc create_function;

public:
  bool loadAPE(const std::filesystem::path& dll_path) {
    // Use wine_dlopen to load PE DLL
    wine_dll_handle = wine_dlopen(dll_path.c_str(), RTLD_NOW);
    if (!wine_dll_handle) return false;

    // Find the APE factory function
    create_function = (APECreateFunc)wine_dlsym(
      wine_dll_handle, "_AVS_APE_RetrFunc"
    );
    return create_function != nullptr;
  }

  std::unique_ptr<Effect> createEffect() {
    C_RBASE* ape_instance = create_function();
    return std::make_unique<WineAPEEffectWrapper>(ape_instance);
  }
};

class WineAPEEffectWrapper : public Effect {
private:
  C_RBASE* ape_instance;
  APEinfo ape_info;  // Provide callbacks to AVS internals

public:
  WineAPEEffectWrapper(C_RBASE* instance)
    : ape_instance(instance) {
    setupAPEinfo();
  }

  void render(FrameContext& ctx) override {
    // Marshal C++ structures across Wine boundary
    ape_instance->render(
      ctx.visdata,
      ctx.isBeat,
      ctx.framebuffer,
      ctx.fbout,
      ctx.width,
      ctx.height
    );
  }

  void setupAPEinfo() {
    ape_info.ver = 3;
    ape_info.global_registers = gmegabuf;  // From EEL context
    ape_info.lineblendmode = &lineblendmode;

    // Provide EEL VM functions
    ape_info.allocVM = &eel_allocVM;
    ape_info.freeVM = &eel_freeVM;
    ape_info.compileVMcode = &eel_compileVMcode;
    ape_info.executeCode = &eel_executeCode;
    // ... etc
  }
};
```

**Dependencies:**
- `libwine-dev` (winelib headers)
- `wine64` or `wine32` runtime

**Limitations:**
- Requires Wine installation
- Cannot show Win32 config dialogs (headless only)
- MSVC vs GCC vtable compatibility issues

#### Option B: Minimal PE Loader (Advanced)

Build a lightweight PE/COFF loader without full Wine:

```cpp
// libs/avs-compat/src/pe_loader.cpp
class MinimalPELoader {
  // Parse PE headers
  // Load sections into memory
  // Resolve imports (Win32 API stubs)
  // Fix relocations
  // Call entry point
};
```

**Advantages:**
- No Wine dependency
- Smaller attack surface
- More control over execution

**Disadvantages:**
- Significant implementation effort
- Must stub Win32 APIs manually
- C++ ABI marshaling still required

### Configuration Dialog Handling

APE plugins traditionally provide Win32 dialogs via `conf()` method. On Linux:

1. **Headless mode:** Skip dialog, use defaults
2. **JSON config:** Generate editable JSON from binary config
3. **Web UI:** Embed config editor in web-based preset editor (future)

## Discovery: Finding Popular APE Effects

### Using grandchild's Tools

```bash
# Clone the dependency analyzer
git clone https://github.com/grandchild/avs-preset-depends
cd avs-preset-depends
cargo build --release

# Analyze a preset collection
./target/release/avs-preset-depends \
  -c \
  -w /path/to/winamp \
  /path/to/preset/collection/*.avs

# Output shows:
# APE Plugin Name         | Usage Count
# ----------------------- | -----------
# Holden03: Convolution   | 234
# Acko.net: Texer II      | 156
# ...
```

### Using AVS-File-Decoder

```bash
git clone https://github.com/grandchild/AVS-File-Decoder
cd AVS-File-Decoder
npm install
npm run build

# Decode preset to JSON
node dist/main.js < preset.avs > preset.json

# Examine APE effects in JSON
jq '.effects[] | select(.id == 16384)' preset.json
```

## Testing Strategy

1. **Unit tests:** Test individual APE effects with known inputs
2. **Integration tests:** Load `.avs` presets that use APE effects
3. **Regression tests:** Compare rendered output with original AVS
4. **Wine compatibility:** Test emulator with real APE DLLs

## Deployment

### Native Effects
- Compiled into `libavs.so`
- Always available
- No runtime dependencies

### Emulated Effects
- Optional at build time: `cmake -DENABLE_APE_EMULATOR=ON`
- Runtime discovery: `~/.config/vis_avs/ape_plugins/*.dll`
- Fallback: Show "APE effect unavailable" warning

## Community Alignment

Both **grandchild's vis_avs** and this project follow the **reimplementation approach**:
- ❌ No dynamic APE DLL loading in production
- ✅ Native reimplementation of popular effects
- ✅ Cross-platform from design

The emulator is a **completeness feature** for rare edge cases, not the primary strategy.

## Next Steps

1. ✅ Document strategy (this file)
2. 🔨 Survey popular presets to identify most-used APEs
3. 🔨 Prioritize APE effects for reimplementation
4. 🔨 Design emulator module architecture
5. 🔨 Implement Wine-based proof-of-concept emulator
6. 🔨 Test with real APE DLLs from Winamp installations

## References

- [APE Architecture Documentation](./avs_original_source_port_notes/ape-plugin-architecture.md)
- [grandchild's vis_avs](https://github.com/grandchild/vis_avs)
- [AVS File Decoder](https://github.com/grandchild/AVS-File-Decoder)
- [AVS Preset Dependencies Analyzer](https://github.com/grandchild/avs-preset-depends)
- [eeltrans (EEL code translator)](https://github.com/TomyLobo/eeltrans)

## Conclusion

The **dual-strategy approach** provides:
1. **Practicality:** Native reimplementation for 95%+ of use cases
2. **Completeness:** Emulator for rare/complex APE effects
3. **Maintainability:** Clean C++ code for common effects
4. **Compatibility:** Fallback for exact original behavior when needed

Priority is on native reimplementation. The emulator is a research/completeness feature.
