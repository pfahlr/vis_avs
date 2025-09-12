## General Insturctions
- the path `/docs/avs_original_source` contains the code we are porting from
- these files are for reference only: do not alter, only read from this directory.


## Agents

### Architect
- **Goal**: Maintain overall architecture and ensure separation of platform-agnostic core from platform shims.
- **Responsibilities**:
  - Define module boundaries (`avs-core`, `avs-platform`, `apps/avs-player`, `apps/avs-studio`).
  - Approve new effect and preset-parsing designs.
  - Keep compatibility matrix updated.

### Planner
- **Goal**: Translate architecture into milestones and actionable issues.
- **Responsibilities**:
  - Maintain `DEV_PLAN.md` with phases (M0–M9).
  - Populate GitHub issues with dependencies and labels.
  - Hand tasks to Builder/Test.

### Builder
- **Goal**: Implement code in C++17/CMake, using SDL2, PortAudio, OpenGL, and ns-eel.
- **Responsibilities**:
  - Port effects incrementally (blur, convolution, color maps, etc.).
  - Replace inline asm with portable SIMD intrinsics.
  - Write code in `libs/avs-core` and `libs/avs-platform`.

### Researcher
- **Goal**: Investigate libraries, legacy quirks, and licensing.
- **Responsibilities**:
  - Document AVS preset format and odd behaviors.
  - Evaluate FFT libraries (KissFFT vs FFTW).
  - Report on similar projects (libvisual, projectM).

### Tester
- **Goal**: Guarantee parity and prevent regressions.
- **Responsibilities**:
  - Create golden-frame tests with deterministic WAV input.
  - Add CI checks for Linux (x86_64, aarch64).
  - Maintain coverage report of supported effects.

### UX
- **Goal**: Provide usable UI for `avs-player` and `avs-studio`.
- **Responsibilities**:
  - Implement Dear ImGui panels (preset browser, EEL editor).
  - Design minimal player UI (hot-reload presets).
  - Ensure cross-platform portability.

### Integrator
- **Goal**: Keep the repo stable, reviewed, and documented.
- **Responsibilities**:
  - Review PRs and enforce code style.
  - Merge only if tests pass in CI.
  - Maintain risk register (`docs/risks.md`).

---

## Collaboration Protocol

1. **Planner** creates or updates issues with `agent:` labels.  
2. **Builder** implements; **Tester** validates.  
3. **Integrator** merges; **Architect** reviews design-affecting changes.  
4. **Researcher** supports when unknowns block Builder/Planner.  
5. **UX** coordinates with Builder on ImGui/SDL interfaces.  

---

## Setup Commands

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build . -j$(nproc)

# Run minimal player
./apps/avs-player/avs-player
````

Dependencies (Linux):

* SDL2 development headers
* PortAudio
* OpenGL / Mesa
* CMake ≥ 3.22
* C++17 compiler (GCC 11+, Clang 13+)

---

## Code Style

* **Standard**: C++17, warnings-as-errors (`-Wall -Wextra -Werror`).
* **Formatting**: `clang-format` (Google style with 100-char line width).
* **Naming**:

  * Classes: `PascalCase`
  * Methods/functions: `camelCase`
  * Constants: `ALL_CAPS`
  * Namespaces: `lowercase`
* **Comments**: Doxygen-style for public headers.
* **Commit messages**: Conventional Commits (`feat:`, `fix:`, `docs:`, etc.).

---

## Dev Environment Tips

* Use `ccache` to speed up rebuilds.
* For live preset reloads, run `avs-player` with `--watch ./presets`.
* For debugging:

  * Run `cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON`
  * Use `gdb ./apps/avs-player/avs-player` or `lldb` as preferred.
* VSCode: install **CMake Tools** + **clangd** extensions.
* Run `pre-commit install` for lint/format hooks.

---

## Testing Instructions

```bash
# From build directory
ctest --output-on-failure

# Run deterministic renderer test
./apps/avs-player/avs-player --headless --wav ../testdata/test.wav --frames 300
# Check golden hashes
diff results/hashes.txt ../testdata/golden_hashes.txt
```

* Unit tests: GoogleTest in `tests/`.
* Integration tests: deterministic WAV → golden frame hash comparison.
* CI: GitHub Actions runs matrix builds on Linux x86\_64 and aarch64 (QEMU).


## PR Instructions

1. Create a feature branch from `main`.

   ```bash
   git checkout -b feat/blur-effect
   ```
2. Ensure code builds and passes tests.

   ```bash
   cmake --build . && ctest
   ```
3. Run `clang-format -i` on modified files.
4. Update relevant docs (`README.md`, `docs/compat.md`).
5. Push and open PR with:

   * Clear description of feature/fix.
   * Screenshots if UI-related.
   * Reference issue number.
6. PR will be reviewed by **Integrator** and **Architect**.
7. CI must pass before merge.

