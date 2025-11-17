# AVS Original Source Port Notes

## Overview

This directory contains comprehensive documentation about the original AVS (Advanced Visualization Studio) source code architecture, design decisions, and porting considerations. These documents analyze the legacy codebase to extract lessons for our modern C++17 cross-platform port.

## Documentation Index

### Core Architecture Documents

1. **[APE Plugin Architecture](./ape-plugin-architecture.md)**
   - Original third-party effect SDK design
   - Why it didn't gain adoption
   - Lessons for building a modern extensibility system
   - Future SDK design recommendations

2. **[NS-EEL Expression Language](./ns-eel-expression-language.md)**
   - AVS's embedded scripting language with JIT compilation
   - How effects like SuperScope, Dynamic Movement use EEL
   - Performance characteristics and security considerations
   - Porting notes for our modern implementation

3. **[Laser Effects Architecture](./laser-effects-architecture.md)**
   - Vector graphics rendering for physical laser hardware
   - Pangolin QM2000 integration and line list management
   - Coordinate system differences from framebuffer rendering
   - Why laser mode exists and future revival options

4. **[Evallib Expression Evaluator](./evallib-expression-evaluator.md)**
   - Original (deprecated) expression library
   - Why it was replaced by NS-EEL
   - Historical context and technical limitations
   - LEX/YACC-based compiler architecture

## Quick Reference

### What Each Component Does

| Component | Status in Modern Port | Purpose |
|-----------|----------------------|---------|
| **NS-EEL** | ✅ **Ported** (`libs/third_party/ns-eel/`) | JIT-compiled scripting for programmable effects |
| **APE SDK** | ❌ **Not Ported** (binary parsers only) | Third-party plugin system (DLL loading) |
| **Laser Mode** | ❌ **Not Ported** (binary parsers only) | Vector graphics for physical laser projectors |
| **Evallib** | ❌ **Obsolete** (replaced by NS-EEL) | Original expression evaluator (pre-JIT era) |

### Binary Preset Coverage

Our modern port can **load all 56 effect types** from original AVS preset files:

- ✅ **46 built-in effects** (IDs 0-45)
- ✅ **5 APE effects** (IDs >= 16384): Channel Shift, Color Reduction, Multiplier, Multi Delay, Video Delay
- ✅ **5 laser effects**: Beat Hold, Brennan's Effect, Moving Cone, Moving Line, Transform

**Total: 56/56 (100% preset compatibility)**

Binary parsers preserve effect data even when full runtime implementation doesn't exist (e.g., laser mode).

## Key Insights for Port Development

### 1. Extensibility Architecture

The original APE SDK's failures teach us what **not** to do:
- ❌ Platform-specific APIs (Win32 HWND, DLL exports)
- ❌ Binary ABI fragility (C++ virtual tables across DLLs)
- ❌ Manual binary serialization (error-prone, no versioning)
- ❌ Limited documentation and examples

**Modern alternatives**:
- ✅ C API with stable ABI or WASM
- ✅ JSON/TOML configuration with schema validation
- ✅ Cross-platform from day one
- ✅ Rich ecosystem (package manager, testing, CI)

### 2. Scripting Integration

NS-EEL's design is elegant:
- **JIT compilation** for native-speed execution
- **Tight integration** with rendering pipeline (pointer-based variables)
- **Minimal syntax** (doubles only, C-like operators)
- **Zero dependencies** (50KB of C code)

This proves that **embedded scripting doesn't require heavy VMs**—when designed for a specific domain (real-time graphics), simplicity wins.

### 3. Multiple Rendering Backends

Laser mode demonstrates **clean separation of concerns**:
- Effects generate **abstract primitives** (pixels vs. vectors)
- Backend handles **output** (framebuffer vs. line list)
- Same effect logic, different render targets

**Lesson**: Design effects to output to an **abstract rendering API** rather than directly manipulating framebuffers. This enables:
- GPU acceleration (translate to shaders)
- Network streaming (compress before sending)
- Recording/export (SVG, video, etc.)

### 4. Evolution Over Revolution

AVS didn't rewrite everything—it evolved:
1. **Evallib → NS-EEL**: Kept similar syntax, added JIT
2. **Framebuffer → Laser**: Dual-mode via `#ifdef`, not full rewrite
3. **Built-in → APE**: Extended rather than replaced

**Takeaway**: Incremental improvement with **backward compatibility** served AVS well. Our port should maintain this philosophy (load old presets, extend gradually).

## Architectural Patterns Observed

### Registry Pattern
Both effect loading and EEL function registration use **registry pattern**:
```cpp
// Original AVS
#define AVS_LEGACY_REGISTER_EFFECT(name, factory) \
    registry[normalize(name)] = factory;

// NS-EEL
NSEEL_addfunc_ret3("lerp", NSEEL_PProc_THIS, &lerp_impl);
```

**Benefits**: Decoupled registration, easy to extend, plugin-friendly.

### Double-Buffering
Framebuffer effects use `fbout` parameter for output:
```cpp
int render(..., int *framebuffer, int *fbout, ...) {
    // Process framebuffer, write to fbout
    return 1;  // fbout has output
}
```

**Prevents**: Read-after-write hazards, enables parallelization.

### Shared State via Global Registers
`reg00`-`reg99` and `gmegabuf[]` enable **inter-effect communication**:
```eel
// Effect 1
reg00 = bass_intensity;

// Effect 2 (later in chain)
scale = reg00 * 2;
```

**Critique**: Global mutable state is fragile. Modern alternatives:
- Named parameters (explicit dependencies)
- Effect composition (output of one → input of next)
- Reactive programming (observables)

## Historical Context

### Timeline Reconstruction

| Era | Technology | Key Features |
|-----|-----------|--------------|
| **AVS 1.x** (2000-2001) | Evallib | Basic expressions, interpreter |
| **AVS 2.x** (2002-2004) | NS-EEL, APE SDK | JIT compilation, plugin system |
| **AVS 2.8+** (2005-2007) | Laser mode | Vector output for Pangolin hardware |

### Community Impact

AVS's **programmability** (NS-EEL) + **preset sharing** created a vibrant ecosystem:
- Thousands of user-generated presets
- Online preset databases (AVS-Archive, Winamp forums)
- Visual programming tutorials (EEL syntax guides)
- Demo scene aesthetic influence

The **combination** of built-in effects (easy) + scriptable effects (powerful) hit a sweet spot for creative experimentation.

## Unanswered Questions

Some aspects of the original codebase remain mysterious:

1. **Why 100 global registers?**
   - Arbitrary limit? Memory constraint?
   - Registry pattern would have allowed unlimited named registers

2. **Why separate `gmegabuf` and `megabuf`?**
   - Global vs. effect-local makes sense, but inconsistent naming
   - Why not `megabuf` and `localbuf`?

3. **Why didn't APEs use NS-EEL for config?**
   - APEs could have exposed EEL variables instead of binary blobs
   - Missed opportunity for user customization

4. **What happened to `r_transition.cpp`?**
   - Infrastructure file, not an effect type
   - Why count it in "56 effects"? (Actually 53 unique + 5 laser)

These mysteries reflect AVS's **organic evolution**—it wasn't designed all at once, it accreted features over years of development.

## Recommended Reading Order

For understanding the original architecture:

1. **Start here**: [NS-EEL Expression Language](./ns-eel-expression-language.md)
   - Core technology powering AVS's uniqueness

2. **Then**: [APE Plugin Architecture](./ape-plugin-architecture.md)
   - Extensibility lessons (what to avoid, what to keep)

3. **Optional**: [Laser Effects Architecture](./laser-effects-architecture.md)
   - Interesting niche feature, demonstrates multiple backends

4. **Historical**: [Evallib Expression Evaluator](./evallib-expression-evaluator.md)
   - Shows how AVS evolved, but not critical for modern port

## Contributing to These Docs

If you discover new insights about the original AVS codebase:

1. Add a new `.md` file in this directory
2. Update this README's index
3. Cross-reference related documents
4. Include code examples from original source when relevant
5. Explain **why** it matters for the modern port

**Format**: Aim for **readable prose**, not dry reference material. Tell the **story** of AVS's architecture.

## Related Resources

- **Project-wide documentation**: `/home/user/vis_avs/CODE_ATLAS.md`
- **Original source code**: `/home/user/vis_avs/docs/avs_original_source/vis_avs/`
- **Modern port code**: `/home/user/vis_avs/libs/`
- **Job specifications**: `/home/user/vis_avs/codex/jobs/`

---

**Last Updated**: 2025-11-17
**Coverage**: APE SDK, NS-EEL, Laser Mode, Evallib
**Status**: Comprehensive analysis complete
