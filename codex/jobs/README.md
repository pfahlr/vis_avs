# Codex Jobs - Development Roadmap

This directory contains YAML job definitions tracking the development progress of vis_avs.

## Job Status Overview

| Status | Count | Jobs |
|--------|-------|------|
| DONE | 13 | 00-12 |
| TODO | 12 | 13-24 |

## Completed Jobs (00-12)

### Foundation & Build System
- **00** - Bootstrap Codex session
- **01** - Fix NSEEL prototypes warnings
- **02** - Fix SSE cast-align warnings
- **03** - Fix PortAudio vendor warnings
- **04** - Fix GL pointer casts warnings

### Preset System
- **05** - Preset GUI/CLI tools
- **06** - Binary preset parser foundation
- **07** - Extended header fix

### Audio Pipeline
- **08** - Audio resampling negotiation
- **09** - PortAudio underflow surfacing
- **10** - WAV file input support
- **11** - Input device selection

### Scripting
- **12** - EEL scripted effect parity

---

## New Jobs (13-24) - Post-Audit Roadmap

### Phase 1: Critical Fixes (Week 1)

#### 13 - Fix Effects Manifest Duplicates 🔴 CRITICAL
- **Labels**: data, manifest, critical
- **Goal**: Remove duplicate effect IDs from effects_manifest.yaml
- **Issues**: trans/color_modifier appears twice, trans/color_clip has conflicting source files
- **Blocks**: Job 14, 24

#### 24 - Consolidate Registry System 🟡 CLEANUP
- **Labels**: cleanup, architecture, technical-debt
- **Goal**: Resolve dual registration (RegisterEffects.cpp vs register_all.cpp)
- **Current**: 111 factories in one file, 1 factory in another (stub?)

---

### Phase 2: Preset Compatibility (Weeks 2-3)

#### 14 - Complete Binary Preset Parser 🔴 CRITICAL
- **Labels**: presets, parser, compatibility, critical
- **Goal**: Expand coverage from 2/56 to 56/56 effect types
- **Impact**: Enables loading original AVS presets
- **Reference**: AVS2K25 files.cpp for proven implementation
- **Blocks**: Jobs 15-17, 23

---

### Phase 3: Missing Effect Implementations (Weeks 3-5)

#### 15 - Implement Transition Effect 🟡 HIGH
- **Labels**: effects, render, trans, high-priority
- **Goal**: 15 blend modes for transitioning between effect chains
- **Usage**: One of most common effects in AVS presets
- **Depends**: Job 14

#### 16 - Implement Superscope Effect 🟡 HIGH
- **Labels**: effects, render, scripting, high-priority
- **Goal**: Programmable waveform effect with EEL scripting
- **Complexity**: 850+ lines in original r_superscope.cpp
- **Depends**: Job 14

#### 17 - Implement Core Missing Effects 🟢 MEDIUM
- **Labels**: effects, render, trans, medium-priority
- **Goal**: Mirror, Invert, Fadeout, Text, OnBeat Clear, Clear Screen, Picture, Interleave
- **Count**: 8 effects total
- **Depends**: Jobs 15, 16

---

### Phase 4: Architecture Improvements (Weeks 4-6)

#### 18 - JSON Preset Format Support 🟢 FEATURE
- **Labels**: presets, format, architecture
- **Goal**: Human-readable preset format with bidirectional conversion
- **Benefits**: Version control, hand-editing, debugging
- **Depends**: Job 14

#### 19 - Multi-threaded Rendering 🟢 PERFORMANCE
- **Labels**: rendering, performance, architecture
- **Goal**: 2-4x speedup via work distribution across CPU cores
- **Pattern**: smp_render(thread_id, max_threads, context)
- **Depends**: Job 17

#### 20 - Pipewire Audio Backend 🟢 PLATFORM
- **Labels**: audio, linux, platform
- **Goal**: Modern Linux audio (successor to PulseAudio/JACK)
- **Benefits**: Lower latency, better device management
- **Reference**: AVS2K25 Pipewire implementation
- **Depends**: Job 11

#### 22 - Framebuffer Abstraction 🟢 ARCHITECTURE
- **Labels**: rendering, architecture, vulkan
- **Goal**: Support OpenGL, Vulkan, CPU, file output backends
- **Benefits**: Video encoding, headless testing, future Vulkan migration
- **Depends**: Job 19

---

### Phase 5: User-Facing Features (Week 6+)

#### 21 - Studio UI MVP 🟢 UI
- **Labels**: ui, studio, user-facing
- **Goal**: Interactive preset editor (currently 84-byte stub)
- **Framework**: ImGui (recommended - immediate mode, SDL2 compatible)
- **Features**: Effect tree, parameter editing, live preview
- **Depends**: Jobs 17, 18

---

### Phase 6: Integration & Validation (Week 7-8)

#### 23 - End-to-End Preset Validation 🎯 MILESTONE
- **Labels**: integration, validation, milestone
- **Goal**: Load and render 20+ original AVS presets correctly
- **Success**: 18+/20 presets render without critical errors (>90%)
- **Deliverable**: Showcase video demonstrating vis_avs
- **Depends**: Jobs 14, 17, 19

---

## Dependency Graph

```
00 (bootstrap) ✓
 └─> 01 (nseel warnings) ✓
      └─> 02 (sse warnings) ✓
           └─> 03 (portaudio warnings) ✓
                └─> 04 (gl warnings) ✓
                     ├─> 05 (preset tools) ✓
                     │    └─> 06 (binary parser) ✓
                     │         ├─> 07 (header fix) ✓
                     │         │    └─> 08 (audio resample) ✓
                     │         │         └─> 09 (pa underflow) ✓
                     │         │              └─> 10 (wav input) ✓
                     │         │                   └─> 11 (device select) ✓
                     │         │                        ├─> 12 (eel parity) ✓
                     │         │                        └─> 20 (pipewire) ⏳
                     │         └─> 12 (eel parity) ✓
                     │              └─> 13 (fix manifest) ⏳
                     │                   ├─> 14 (complete parser) ⏳
                     │                   │    ├─> 15 (transition) ⏳
                     │                   │    │    └─> 17 (core effects) ⏳
                     │                   │    │         ├─> 19 (multithreading) ⏳
                     │                   │    │         │    ├─> 22 (framebuffer) ⏳
                     │                   │    │         │    └─> 23 (validation) ⏳
                     │                   │    │         └─> 23 (validation) ⏳
                     │                   │    ├─> 16 (superscope) ⏳
                     │                   │    │    └─> 17 (core effects) ⏳
                     │                   │    ├─> 18 (json presets) ⏳
                     │                   │    │    └─> 21 (studio ui) ⏳
                     │                   │    └─> 23 (validation) ⏳
                     │                   └─> 24 (registry cleanup) ⏳
                     └─> 13 (fix manifest) ⏳

Legend: ✓ DONE | ⏳ TODO
```

## Priority Matrix

| Priority | Jobs | Est. Weeks | Impact |
|----------|------|------------|--------|
| 🔴 CRITICAL | 13, 14 | 1-2 | Unblocks everything else |
| 🟡 HIGH | 15, 16 | 2 | Core functionality for presets |
| 🟢 MEDIUM | 17, 18, 19, 20, 22, 24 | 3-4 | Quality of life & performance |
| 🟢 LOW | 21 | 1-2 | User-facing polish |
| 🎯 MILESTONE | 23 | 1 | Integration validation |

## Timeline Estimate

- **Weeks 1-2**: Jobs 13, 14, 24 (critical fixes & parser completion)
- **Weeks 3-4**: Jobs 15, 16, 17 (effect implementations)
- **Weeks 5-6**: Jobs 18, 19, 20, 22 (architecture improvements)
- **Week 7**: Job 21 (UI MVP)
- **Week 8**: Job 23 (validation & demo)

**Total**: ~8 weeks to feature parity with original AVS

## Quick Start

To work on a job:
1. Read the job YAML file
2. Update status: `TODO` → `IN_PROGRESS` → `IN_REVIEW` → `DONE`
3. Follow steps and acceptance criteria
4. Run test_hook commands to validate
5. Open PR referencing job ID

## Job File Format

```yaml
id: unique-job-identifier
title: Human-readable job title
status: TODO | IN_PROGRESS | IN_REVIEW | DONE
labels: [category1, category2]
depends_on:
  - prerequisite-job-id
goal: >
  Multi-line description of what this job aims to achieve
steps:
  - Ordered list of concrete actions
acceptance_criteria:
  - Testable conditions for job completion
test_hook:
  - Commands to validate job completion
files_touched:
  - List of files expected to change
```

## Notes

- Jobs 00-12 focused on build system, warnings, and audio pipeline
- Jobs 13-24 focus on effect completeness and architecture
- All new jobs created from comprehensive codebase audit (2025-11-17)
- See audit report for detailed analysis and rationale
