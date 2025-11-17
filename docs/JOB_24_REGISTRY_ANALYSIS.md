# Registry Consolidation Analysis (Job 24)

## Current Registration Systems

After investigation, I've identified **three distinct registration systems** in vis_avs:

### 1. Modern Core Registry (`avs::core::EffectRegistry`)
- **Location**: `libs/avs-core/`
- **Population**: `avs::effects::registerCoreEffects()` in `RegisterEffects.cpp`
- **Usage**: Unit tests, modern effect instantiation
- **Effects**: 111 factory registrations (56 effects with name variants)
- **Interface**: Effects inherit from `avs::core::IEffect`

### 2. Legacy Binary Parser Registry (avs::effects::legacy::EffectFactory)
- **Location**: `libs/avs-effects-legacy/src/effect_registry.cpp`
- **Population**: `AVS_LEGACY_REGISTER_EFFECT` macro (static initialization)
- **Usage**: Binary .avs preset parsing
- **Effects**: 54 effects registered
- **Interface**: Returns `avs::Effect` (compat layer)
- **Purpose**: Parse binary payload and create effect instances

### 3. Modern Preset Registry (`avs::effects::Registry`)
- **Location**: `libs/avs-effects-legacy/src/register_all.cpp` (stub)
- **Population**: `register_all()` function calls `register_*()` helpers
- **Usage**: Preset binding, effect instantiation from IR
- **Effects**: Only 1 implemented (color_modifier)
- **Interface**: Effects inherit from `avs::effects::legacy::IEffect`
- **Status**: **INCOMPLETE** - only stub implementation exists

## The Problem

The job specification identifies confusion between:
- `RegisterEffects.cpp` - 111 factories for modern core registry
- `register_all.cpp` - 1 factory stub for preset registry

However, these serve **different purposes**:
- `RegisterEffects.cpp` → `avs::core::EffectRegistry` (modern system)
- `register_all.cpp` → `avs::effects::Registry` (preset system)

## Investigation Findings

### RegisterEffects.cpp Usage
Used extensively in tests (11+ files):
- `tests/core/test_blend_ops.cpp`
- `tests/core/test_globals_and_bump.cpp`
- `tests/presets/dynamic/test_dynamic.cpp`
- And 8+ more test files

**Not used in production apps** (apps/avs-player doesn't call it directly).

### register_all.cpp Usage
Used in 3 test files:
- `libs/avs-effects-legacy/tests/test_registry.cpp`
- `libs/avs-preset/tests/test_ir_and_binder.cpp`
- Referenced in `docs/preset_bind_registry.md`

**Potentially used in preset loading** but only has 1 effect implemented.

### effect_registry.cpp (AVS_LEGACY_REGISTER_EFFECT)
Uses static initialization to register 54 effects for binary preset parsing.
**This is production code** used when loading .avs files.

## Recommended Action Plan

### Option A: Complete register_all() System (Recommended)
1. Determine if `avs::effects::Registry` is actually needed for production
2. If YES: Implement all 56 `register_*()` functions (one per effect)
   - Follow pattern from `effect_color_modifier_registry.cpp`
   - Create bridge between `avs::core::IEffect` and `avs::effects::legacy::IEffect`
3. If NO: Remove `avs::effects::Registry` entirely and migrate to core registry

### Option B: Remove register_all() Stub
1. Delete `register_all.cpp` and `register_all.h`
2. Update tests to use `registerCoreEffects()` instead
3. Ensure preset binding works without `avs::effects::Registry`
4. Document that `avs::core::EffectRegistry` is the canonical system

## Blockers

1. **Architecture Uncertainty**: Need to understand if `avs::effects::Registry` is required
   for production preset loading or if it's test-only code.

2. **Multiple Interfaces**: Three different IEffect interfaces exist:
   - `avs::core::IEffect` (modern)
   - `avs::Effect` (compat layer)
   - `avs::effects::legacy::IEffect` (preset system)

3. **Incomplete Specification**: The pattern for implementing `register_*()` functions
   is only demonstrated for one effect (color_modifier).

## Next Steps

1. **Trace preset loading** in avs-player to see which registry is actually used
2. **Consult with maintainer** about intended architecture
3. **Make decision**: Complete vs. Remove
4. **Implement chosen solution** systematically

## Time Estimate

- Investigation & Decision: 2 hours ✅ (completed)
- Implementation (if completing): 8-12 hours
- Implementation (if removing): 2-4 hours
- Testing & Documentation: 2-3 hours

**Total**: 6-17 hours depending on approach

## Status

**BLOCKED** - Requires architectural decision before proceeding.

Recommend pausing Job 24 and focusing on Jobs 22 and 23 which have clearer paths.
