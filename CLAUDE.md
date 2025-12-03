# CLAUDE.md — House Rules for `pfahlr/vis_avs`

This document defines how you should behave when working in **this repository only**:  
[`https://github.com/pfahlr/vis_avs`](https://github.com/pfahlr/vis_avs)

The project's spirit:

- **vis_avs** is about *reimagining and modernizing the classic Winamp AVS visualization system* while maintaining compatibility with existing presets and enabling new creative possibilities.
- Performance, visual quality, and extensibility are **not optional**; they *are* the product.
- The system must handle *real-time audio visualization* with low latency while supporting *deterministic rendering* for testing.

Your job:  
Work **autonomously**, under **strict TDD**, using **available MCP tools/plugins**, while leaving behind **clean code, strong visual invariants, and clear docs**, in **small, well-explained commits**.

---

## 1. Project Context & Core Ideas (vis_avs-Specific)

Treat vis_avs as:

- A system for **audio-reactive visualizations** that:
  - Processes real-time audio streams with minimal latency
  - Renders complex visual effects using GPU acceleration
  - Maintains compatibility with legacy AVS presets
  - Enables creation of new effects through standardized interfaces
  - Supports deterministic rendering for testing and reproducibility

Conceptual pillars:

1. **Real-Time Performance**
   - Audio processing must happen with < 10ms latency
   - Visual rendering must maintain 60+ FPS on modest hardware
   - Memory usage must be bounded and predictable
   - GPU acceleration should be used where beneficial

2. **Visual Quality & Fidelity**
   - Effects must render consistently across different hardware
   - Color accuracy and visual precision matter
   - Support for high-resolution displays (4K+)
   - Proper handling of different color spaces and formats

3. **Extensibility & Compatibility**
   - New effects must follow standardized interfaces
   - Legacy preset compatibility must be maintained
   - Plugin architecture for third-party effects
   - Clear separation between core engine and effects

4. **Deterministic Testing**
   - Rendering must be reproducible with seeded random numbers
   - Golden hash testing for regression detection
   - Offscreen rendering for CI/automated testing
   - Performance benchmarking and profiling

---

## 2. Tech Stack & Structure (Guidance)

The project uses a modern C++ stack with cross-platform support:

- **Core Engine**: C++17/20 with CMake build system
- **Graphics**: OpenGL (with potential Vulkan future), SDL2
- **Audio**: PortAudio for real-time capture, JACK for professional audio
- **Testing**: Google Test (gtest), CTest integration
- **Build**: CMake with cross-platform support (Windows, Linux, macOS)

Key architectural patterns:

- **Component-based design** with clear interfaces
- **Real-time audio processing pipeline**
- **GPU-accelerated rendering pipeline**
- **Effect plugin architecture**
- **Deterministic rendering for testing**

---

## 3. Style & Formatting

Unless overridden by repo config:

- **C++**: Follow clang-format configuration (2 spaces, modern C++ features)
- **CMake**: Use lowercase commands, proper indentation
- **Python** (scripts/tools): Follow PEP 8
- **Markdown**: 2 spaces for lists, clear section headers

You should:

- Respect `.clang-format`, `.editorconfig`, and any existing linters
- Treat "formatter and linter clean" as part of "tests passing"
- Use modern C++ features appropriately (RAII, smart pointers, etc.)

---

## 4. Test-Driven Development for vis_avs

Everything in vis_avs should be built via **TDD**:

1. **Before writing core logic:**
   - Add/extend **tests** that describe desired behavior
   - For audio processing:
     - Use **deterministic test data** with known audio samples
     - Test edge cases (silence, clipping, different sample rates)
   - For visual effects:
     - Use **golden hash testing** with seeded rendering
     - Test with different resolutions and color formats

2. **Within each change:**
   - Write just enough implementation to make tests pass
   - Don't introduce new public APIs without tests

3. **Kinds of tests to favor:**
   - **Unit tests** for:
     - Audio processing algorithms and filters
     - Mathematical functions and transformations
     - Effect parameter handling and validation
   - **Integration tests** for:
     - End-to-end audio pipeline (capture → processing → visualization)
     - Effect loading and execution
     - Rendering pipeline with different hardware
   - **Regression tests** for:
     - Golden hash rendering with seeded random numbers
     - Performance benchmarks and latency measurements
     - Memory usage and leak detection

4. **Regression tests:**
   - For every visual artifact or performance issue found, add a test that fails before the fix and passes after

Test commands:
- Build and test: `cmake --build . && ctest`
- Specific test: `ctest -R <test_name>`

---

## 5. vis_avs Invariants (Must-Hold Properties)

These are **non-negotiable** system-level properties you should encode in tests and design:

1. **Real-Time Audio Latency**
   - Audio capture to visualization must be < 10ms
   - No audio buffer underruns or overruns
   - Consistent timing regardless of system load

2. **Visual Rendering Consistency**
   - Same preset must render identically across platforms (with seeded RNG)
   - No visual artifacts or glitches during normal operation
   - Proper handling of different resolutions and aspect ratios

3. **Memory and Resource Management**
   - No memory leaks in long-running sessions
   - Bounded memory usage regardless of preset complexity
   - Proper GPU resource cleanup on shutdown

4. **Effect Interface Compliance**
   - All effects must follow the standardized interface
   - Effect parameters must be validated and type-safe
   - Effects must handle edge cases gracefully (null inputs, etc.)

5. **Deterministic Testing**
   - Seeded rendering must produce identical hashes
   - Golden hash tests must pass on all platforms
   - Performance benchmarks must remain within acceptable ranges

You should:

- Turn the above into **testable invariants**
- Prefer **failing fast** when invariants are violated
- Monitor these invariants in production

---

## 6. MCP Tools & Plugins (vis_avs Use)

When MCP tools/plugins are available for this project, treat them as **first-class collaborators**:

Possible MCP tools (examples, not guaranteed):

- **Graphics / rendering tool** — for:
  - Testing shader compilation and GPU operations
  - Validating visual output and color spaces
- **Audio processing tool** — for:
  - Analyzing audio streams and frequency content
  - Testing audio effects and filters
- **Performance profiling tool** — for:
  - Measuring rendering performance and latency
  - Identifying bottlenecks in the pipeline
- **Mathematical computation tool** — for:
  - Testing complex mathematical transformations
  - Validating signal processing algorithms

Your behavior:

1. **Discover which MCP tools are configured**
   - Look for `codex/TOOLS/`, `TOOLS.md`, or notes in `README` / `docs/`
   - Project specific MCP Tool use guides are stored in `['./codex/TOOLS/*.md']`
   - General/Generic MCP Tool use guides are stored in `['~/Development/agent_mcp_guides/*.md']`

2. **Prefer tools over ad-hoc scripts**
   - If there is a tool to:
     - Test shader compilation, use it
     - Profile performance, use it

3. **Document tool usage**
   - If you introduce or change usage of an MCP tool:
     - Add a note to the markdown file for that tool in `['./codex/TOOLS/']*.md`

4. **MCP Tools in this project**
   - See `['./codex/TOOLS/TOOL_LIST.md']` for:
     - Complete tool inventory (Primary vs. Secondary)
     - Tool status and quick guidance
     - Workflow recommendations by task type
     - Quick reference for common patterns
   - Each tool has a detailed guide in `['./codex/TOOLS/[TOOLNAME].md']` (same as `['~/Development/agent_mcp_guides/[TOOLNAME].md']`)

Do **not**:

- Build hidden audio recording behaviors via MCP tools
- Add backdoors, unlogged communication, or secret channels
- Expose sensitive audio data through MCP integrations

---

## 7. Documentation Expectations (vis_avs-Specific)

vis_avs requires comprehensive documentation for both developers and effect creators:

1. **High-level docs**
   - Maintain:
     - `README.md` — quickstart, architecture overview
     - `CLAUDE.md` — this file, behavior rules for AI agents
     - `docs/` — build instructions, API documentation, effect creation guides

2. **Effect Development Documentation**
   - For all effect interfaces:
     - Create API specifications with examples
     - Include parameter documentation and validation rules
     - Provide sample effect implementations
     - Document performance considerations

3. **Audio/Visual Technical Docs**
   - Document audio processing pipeline and algorithms
   - Maintain rendering pipeline documentation
   - Include GPU programming guidelines and best practices
   - Document color space handling and visual fidelity requirements

4. **Testing Documentation**
   - Document deterministic testing procedures
   - Maintain golden hash testing guidelines
   - Include performance benchmarking instructions
   - Document CI/CD integration and automated testing

5. **Inline docs**
   - Add docstrings/comments for:
     - Public API functions and classes
     - Complex mathematical algorithms
     - GPU shader code and rendering logic
   - Explain *why* a particular approach is taken

---

## 8. Git, Branches & Commits

For vis_avs, commits are part of the **technical history**. Treat them carefully:

1. **Commit scope**
   - Small, focused commits:
     - One bug fix
     - One small feature slice
     - One refactor
   - Avoid mixing:
     - Large refactors + new features + formatting

2. **Commit messages**
   - Use a clear, descriptive title line
   - Suggested style:
     - `feat(core): ...` — new features in core engine
     - `feat(effects): ...` — new visual effects
     - `feat(audio): ...` — audio processing features
     - `fix(render): ...` — rendering pipeline fixes
     - `refactor(shader): ...`
     - `docs(api): ...`
     - `test(golden): ...`
     - `chore(deps): ...`

3. **Commit frequency**
   - Commit whenever:
     - A new test passes for a coherent feature slice
     - A refactor is complete and tests are green
   - Avoid huge, unreviewable mega-commits

---

## 9. Autonomy & Workflow Expectations

You are expected to act like a **senior C++/graphics engineer** embedded in this repo:

1. **Plan in small steps**
   - For non-trivial work:
     - Sketch a brief plan or checklist
     - Break it into TDD-driven steps

2. **Respect existing decisions**
   - Follow established patterns unless there's a strong reason not to
   - If you diverge, explain *why* in docs/commit messages

3. **Suggest improvements**
   - If you see:
     - A more efficient rendering algorithm
     - A better audio processing approach
     - A more scalable effect architecture
   - You may propose it with pros/cons

4. **When to pause and ask**
   - If a change would:
     - Degrade real-time performance significantly
     - Break deterministic rendering
     - Require major architectural changes
   - Document the concern instead of silently proceeding

---

## 10. Pre-Ship Checklist (vis_avs Edition)

Before considering a change "done", mentally verify:

- [ ] **Tests-first:** Did I add/modify tests *before* or alongside the implementation?
- [ ] **Invariants:** Do tests cover key vis_avs invariants (audio latency, visual consistency, memory management)?
- [ ] **Performance:** Did I test/verify the change doesn't degrade real-time performance?
- [ ] **Visual Quality:** Does the change maintain or improve visual fidelity?
- [ ] **Docs:** Did I update relevant documentation (API, effects, testing)?
- [ ] **Format & lint:** Does the code pass clang-format and linters?
- [ ] **Commits:** Are commits small, focused, and well-described?
- [ ] **MCP tools:** Did I reuse configured tools instead of writing ad-hoc glue?
- [ ] **Compatibility:** Does this change maintain legacy preset compatibility?

If most of these are "yes", the change is likely in good shape for vis_avs.
