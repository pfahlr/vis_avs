# Job 22a: RenderContext Integration - COMPLETE ✅

## Summary

Successfully integrated IFramebuffer abstraction into RenderContext, making framebuffer backends accessible to all effects. This is the **critical path** for enabling backend selection (CPU, OpenGL, File export).

## Implementation

### 1. RenderContext Update

**File**: `libs/avs-core/include/avs/core/RenderContext.hpp`

Added `IFramebuffer* framebufferBackend` field to RenderContext:

```cpp
struct RenderContext {
  // ... existing fields ...

  /**
   * @brief Framebuffer backend (modern interface, optional).
   *
   * When set, effects should prefer this over the legacy framebuffer view.
   * Provides upload(), download(), resize(), clear(), and backend metadata.
   * May be nullptr for legacy code paths.
   */
  IFramebuffer* framebufferBackend = nullptr;

  /**
   * @brief Legacy pixel buffer view (deprecated, use framebufferBackend).
   *
   * Maintained for backward compatibility. New effects should use
   * framebufferBackend->data() instead of framebuffer.data directly.
   */
  PixelBufferView framebuffer;

  // ... existing fields ...
};
```

### 2. Backward Compatibility

**Strategy**: Dual-interface approach during transition period.

- **New code**: Use `context.framebufferBackend` (provides upload/download/resize/clear)
- **Legacy code**: Continue using `context.framebuffer` (raw pixel view)
- **Both**: Can coexist during migration period

**Benefits**:
- Zero breaking changes to existing effects
- Gradual migration path (56 legacy effects can be updated incrementally)
- New effects can use modern interface immediately

### 3. Proof of Concept Tests

**File**: `tests/integration/test_rendercontext_framebuffer.cpp`

Created 3 comprehensive tests demonstrating integration:

1. **`ClearEffectUsesFramebufferBackend`**: Validates effects can render using IFramebuffer
   - Creates CPUFramebuffer (64x48)
   - Sets up RenderContext with both backend and legacy view
   - Renders Clear effect
   - Verifies framebuffer state changes correctly

2. **`LegacyFramebufferViewStillWorks`**: Ensures backward compatibility
   - Creates pixel buffer manually (old way)
   - Sets up RenderContext with ONLY legacy view (no backend)
   - Renders Clear effect
   - Verifies old code path still works

3. **`BackendMetadataAccessible`**: Validates backend introspection
   - Queries backend name ("CPU")
   - Checks direct access support
   - Retrieves width/height from backend

**Test Results**: All 3 tests passing ✅

## Migration Path

### For Effect Authors

**Old Way (still supported)**:
```cpp
bool MyEffect::render(RenderContext& context) {
  uint8_t* pixels = context.framebuffer.data;
  // ... manipulate pixels directly ...
}
```

**New Way (recommended)**:
```cpp
bool MyEffect::render(RenderContext& context) {
  if (context.framebufferBackend) {
    // Modern path: use framebuffer backend
    uint8_t* pixels = context.framebufferBackend->data();
    context.framebufferBackend->clear(0, 0, 0, 255);
    // ... or use upload/download for GPU backends ...
  } else {
    // Fallback to legacy path
    uint8_t* pixels = context.framebuffer.data;
  }
}
```

### For Render Pipeline Owners

**Setting up RenderContext with IFramebuffer**:
```cpp
auto framebuffer = avs::core::createCPUFramebuffer(width, height);

avs::core::RenderContext context{};
context.width = width;
context.height = height;
context.framebufferBackend = framebuffer.get();

// For backward compatibility, also set legacy view
context.framebuffer.data = framebuffer->data();
context.framebuffer.size = framebuffer->sizeBytes();

// Render effects
pipeline.render(context);
```

## Next Steps (Job 22b, 22c)

### Job 22b: CLI Backend Selection (1-2 hours)
- Add `--render-backend` option to avs-player
- Allow users to choose CPU/OpenGL/File backends
- Update OffscreenRenderer to use IFramebuffer

### Job 22c: Effect Migration (Incremental)
- Gradually update 56 legacy effects to use framebufferBackend
- Low priority (can happen over months/years)
- Effects work fine with legacy view indefinitely

## Testing

### Tests Passing
- ✅ `rendercontext_framebuffer_tests` (3/3 tests)
- ✅ `framebuffer_backend_tests` (15/15 tests)
- ✅ `preset_validation_tests` (30/36 presets, 83.3%)
- ✅ `core_effects_tests` (all tests passing)

### No Regressions
- All existing effects continue to work
- Preset validation still at 83.3% success rate
- No performance impact (pointer added to struct)

## Benefits Achieved

1. **Backend Selection Enabled**: Effects can now access different framebuffer backends (CPU, OpenGL, File)
2. **Zero Breaking Changes**: All existing code works unchanged
3. **Migration Path Defined**: Clear documentation for updating effects
4. **Tested Integration**: 3 comprehensive tests validate the integration
5. **Future-Proof**: Easy to add new backends (Vulkan, Metal, etc.)

## Status

**COMPLETE** ✅

Critical path for backend selection is now open. Job 22b can proceed to add CLI options, and Job 22c can migrate effects incrementally as needed.
