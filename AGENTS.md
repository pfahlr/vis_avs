# AGENTS

## Setup Commands

* **Clone and configure**

```bash
git clone https://github.com/pfahlr/vis_avs.git
cd avs-port
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

* **Build**

```bash
cmake --build . -j"$(nproc)"
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
  * **Note:** If JACK or ALSA backends are missing, install `libjack-dev` and `libasound2-dev`.

**All dependencies listed here must be mirrored in `/.github/workflows/ci.yml`.**

---

## Code Style

* **Language:** C++17, compile with `-Wall -Wextra -Werror`
* **Formatting:** `clang-format` (Google style, 100-col width)
* **Naming:**

  * Classes: `PascalCase`
  * Functions/methods: `camelCase`
  * Constants: `ALL_CAPS`
  * Namespaces: `lowercase`
* **Docs:** Doxygen comments for public headers

---

## Dev Environment Tips

* Use `ccache` to accelerate rebuilds.
* Enable sanitizers during dev builds:

  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
  ```
* For deterministic rendering tests, disable vsync (`--headless` flag).
* Pre-commit: run `clang-format -i` and `ctest` before commits.
* IDE: VSCode with **CMake Tools** + **clangd** recommended.

---

## Testing Instructions

* **Unit Tests:** run with `ctest` from the build directory.
* **Integration Tests:** deterministic renderer CLI:

  ```bash
  ./apps/avs-player/avs-player \
    --headless \
    --wav ../testdata/test.wav \
    --preset ../testdata/simple.avs \
    --frames 120
  diff results / hashes with ../testdata/golden/
  ```
* **Golden Frame Tests:** store canonical outputs in `tests/golden/`. CI compares SHA256 frame hashes.
* **Performance Checks:** microbenchmark effects; fail CI if regression > 20% vs baseline.
* **CI Mirrors Local:** keep `docs/README.md` dependencies in sync with `/.github/workflows/ci.yml`.

---

## PR Instructions

* **Branching:** `feat/…`, `fix/…`
* **Commits:** Conventional Commits (`feat:`, `fix:`, `docs:` …)
* **Checklist before PR:**

  * [ ] Code formatted (`clang-format`)
  * [ ] All unit/integration tests pass (`ctest`)
  * [ ] Added/updated tests for new code
  * [ ] Docs updated (README, compat.md, risks.md if relevant)
  * [ ] Dependencies updated in **both** `docs/README.md` and `/.github/workflows/ci.yml`
* **CI Gate (must pass):**

  * Build
  * Tests
  * Formatting
  * Dependency sync

PR template: `.github/pull_request_template.md`

---

## Test-Driven Development Guidance

* **Red → Green → Refactor:** write a failing test, implement, then simplify.
* **Every new feature requires a test:**

  * New effect → unit test + visual checksum integration
  * Parser change → preset round-trip test
  * Audio pipeline change → sine-wave FFT validation
* **No untested code merges.**
* **Determinism is king:** rendering tests must be reproducible across machines and CI.

---

## Task Management Rules for Codex (YAML jobs — no “Start Task” buttons)

**Codex MUST NOT create or present “Start Task” buttons in the UI.**
Instead, Codex MUST create or edit YAML job files under `codex/jobs/` and let automation sync them to GitHub Issues.

### Source of truth

* Each job is a file at `codex/jobs/<id>.yaml`.
* **IDs** are stable, unique, and kebab-case (e.g., `warn-sse-cast-align`).
* Jobs are versioned in git; Issues are derived artifacts (synced by CI).

### YAML schema (required fields)

```yaml
id: <kebab-case-id>
title: <short summary>
status: READY        # TODO | READY | IN-PROGRESS | IN-REVIEW | DONE
labels: [warnings, parser, audio]   # GitHub labels to apply
depends_on: []       # list of other job IDs
goal: >              # one-paragraph outcome/intent
  ...
steps:
  - First concrete step
  - Second step …
acceptance_criteria:
  - Observable criteria the PR must satisfy
test_hook:
  - "How to prove it in CI (cmds/tests)"
files_touched:
  - path/one
  - path/two
```

### Execution protocol for Codex

1. **Intake:** Read `/codex/intake.md` and scan `/codex/jobs/*.yaml`.
2. **Pick:** Choose the first `status: READY` whose `depends_on` are all `DONE` (or empty).
3. **Do:** Implement **exactly** the listed `steps`. Treat `acceptance_criteria` and `test_hook` as gates.
4. **PR:**

   * Use the repo PR template.
   * Map each step to commits or a checklist.
   * Include CI results summary.
   * Follow binary policy below.
5. **Update job status:** In the same PR, change `status: READY` → `IN-REVIEW`.

### Issue sync

* A GitHub Action (`.github/workflows/seed-codex-jobs.yml`) converts each YAML into a GitHub Issue titled `[Codex:<id>] <title>`.
* Codex may reference/close Issues, but **must not** create tasks directly in the Codex UI.

---

## Binary assets in Codex PRs (code-only PRs; binaries via ZIP artifact)

**Goal:** Keep binaries in the repo overall, but **Codex PRs must never include binary diffs**. Codex will attach a ZIP to its chat output; the maintainer opens a companion PR to commit binaries.

### What counts as “binary”

* Any non-text file (e.g., `*.png *.jpg *.gif *.zip *.7z *.mp4 *.avi *.wav *.mp3 *.ttf *.otf *.dll *.so *.dylib *.wasm`)
* Anything `git diff` shows as binary

### Codex workflow when binaries changed

1. **Stage code only.** Unstage binaries while leaving them in the working tree:

   ```bash
   # candidate AM files
   git diff --name-only --diff-filter=AM > /tmp/candidates.txt
   # detect binaries via numstat “- -”
   : > /tmp/binaries.txt
   while read -r p; do
     if git diff --numstat -- "$p" | grep -q '^-'; then echo "$p" >> /tmp/binaries.txt; fi
   done < /tmp/candidates.txt
   xargs -a /tmp/binaries.txt -r git restore --staged --
   ```
2. **Create ZIP artifact** of the **repo-relative paths** (so extracting at repo root places them correctly):

   ```bash
   mkdir -p _codex_artifacts
   zip -q -r "_codex_artifacts/binaries-<branch>-<stamp>.zip" @/tmp/binaries.txt
   ```
3. **Manifest inside ZIP:** `BINARY_ASSETS.manifest.json` listing path, size, sha256 for each file.
4. **Attach ZIP** in the Codex message (out-of-band), include ZIP SHA256.
5. **Open code-only PR** with a **Binary assets** section in the body (paths, sizes, hashes, ZIP name).
6. **Tests depending on binaries:** gate with `GTEST_SKIP()` (or equivalent) if files are absent; instruct maintainer to apply the companion PR locally to run full suite.

> Codex must **not** change `.gitignore` to hide binaries.

---

## CI guardrails (Codex must assume)

* Treat warnings as errors: set `-Werror` in CI for C and C++.
* Run unit + integration tests via `ctest`.
* Enforce formatting check (clang-format).
* Seed Codex jobs to Issues via `.github/workflows/seed-codex-jobs.yml`.

---

## Where Codex finds the playbook

* **Intake & policies:** `codex/intake.md`, `codex/policies.md`
* **Jobs:** `codex/jobs/*.yaml`
* **PR template:** `.github/pull_request_template.md`
* **Issue sync:** `.github/workflows/seed-codex-jobs.yml`

---

## Quick example job (for reference)

```yaml
id: warn-sse-cast-align
title: Fix SSE cast-align warnings
status: READY
labels: [warnings, sse, avs-core]
depends_on: []
goal: >
  Remove -Wcast-align by using unaligned intrinsics or memcpy instead of
  pointer reinterprets to __m128i*.
steps:
  - Replace reinterpret_cast<__m128i*> with _mm_loadu_si128/_mm_storeu_si128 or memcpy.
  - Safe-load 32-bit center value in radial_blur.cpp (memcpy → _mm_set1_epi32).
  - Build avs-core with -Wcast-align as error.
acceptance_criteria:
  - No -Wcast-align diagnostics in avs-core targets under -Werror.
test_hook:
  - "CI adds -Werror -Wcast-align; run ctest."
files_touched:
  - libs/avs-core/src/effects/radial_blur.cpp
  - libs/avs-core/src/effects/motion_blur.cpp
```
