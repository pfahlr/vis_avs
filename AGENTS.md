# AGENTS

## Setup Commands
- **Clone and configure**
  ```bash
  git clone https://github.com/your-org/avs-port.git
  cd avs-port
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Debug
````

* **Build**

  ```bash
  cmake --build . -j$(nproc)
  ```
* **Run**

  ```bash
  ./apps/avs-player/avs-player
  ```
* **Dependencies (Linux, CI and local)**

  * Build essentials: `cmake g++ clang-format git`
  * SDL2: `libsdl2-dev`
  * OpenGL: `mesa-common-dev libglu1-mesa-dev`
  * PortAudio: `portaudio19-dev libportaudio2`
  * KissFFT: vendored (no system package required)
  * GoogleTest: `libgtest-dev` (or FetchContent in CMake)
  * ImGui (optional): vendored
* **Note:** If `jack-audio` or ALSA backends are missing, install `libjack-dev` and `libasound2-dev`.

All dependencies listed here **must** be mirrored in `/.github/workflows/ci.yml`.

---

## Code Style

* **Language**: C++17, compiled with `-Wall -Wextra -Werror`.
* **Formatting**: `clang-format` (Google style, 100-col width).
* **Naming**:

  * Classes: `PascalCase`
  * Functions/methods: `camelCase`
  * Constants: `ALL_CAPS`
  * Namespaces: `lowercase`
* **Docs**: Doxygen comments for public headers.

---

## Dev Environment Tips

* Use `ccache` to accelerate rebuilds.
* Enable sanitizers during dev builds:

  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
  ```
* For deterministic rendering tests, disable vsync (`--headless` flag).
* Pre-commit hooks: run `clang-format -i` and `ctest` before commits.
* IDE: VSCode with **CMake Tools** + **clangd** recommended.

---

## Testing Instructions

* **Unit Tests**: run with `ctest` from the build directory.
* **Integration Tests**: use deterministic renderer CLI:

  ```bash
  ./apps/avs-player/avs-player --headless --wav ../testdata/test.wav --preset ../testdata/simple.avs --frames 120
  diff results/hashes.txt ../testdata/golden/hashes.txt
  ```
* **Golden Frame Tests**:

  * Store canonical outputs in `tests/golden/`.
  * CI compares SHA256 hashes frame-by-frame.
* **Performance Checks**:

  * Microbenchmark effects.
  * Fail CI if regression >20% compared to baseline.
* **CI Mirrors Local**:

  * Any package you install locally must be added to `/.github/workflows/ci.yml`.
  * Keep `docs/README.md` dependency list in sync with CI.

---

## PR Instructions

* **Branching**: create feature branches (`feat/xyz`, `fix/abc`).
* **Commit Style**: Conventional Commits (`feat:`, `fix:`, `docs:`).
* **Checklist** before opening PR:

  * [ ] Code formatted (`clang-format`).
  * [ ] All unit/integration tests pass locally (`ctest`).
  * [ ] Added/updated tests for new code.
  * [ ] Updated docs (README, compat.md, risks.md if relevant).
  * [ ] Dependencies updated in **both** `docs/README.md` and `/.github/workflows/ci.yml`.
* **CI Gate**: PR cannot be merged if:

  * Build fails
  * Tests fail
  * Formatting check fails
  * Dependencies missing in workflow

---

## Test-Driven Development Guidance

* **Red → Green → Refactor**: write a failing test, implement code, then simplify.
* **Every new feature requires a test**:

  * New effect → add effect unit test + visual checksum in integration.
  * Parser change → add preset round-trip test.
  * Audio pipeline change → add sine wave FFT validation test.
* **No untested code merges**: Integrator must reject PRs lacking tests.
* **Determinism is king**: all rendering tests must be reproducible across machines and CI.

---

