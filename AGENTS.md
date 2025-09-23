# AGENTS

## Setup Commands
- **Clone and configure**
```bash
  git clone https://github.com/your-org/avs-port.git
  cd avs-port
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Debug
```

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

## Binary assets in Codex PRs (code-only PRs; binaries via ZIP artifact)

**Goal:** Keep binary assets in the repository, but **do not include them in Codex-generated pull requests**. Instead, Codex must package any changed/added binaries into a ZIP and attach it to its message, while opening a **code-only** PR. I (the maintainer) will open a matching binary-assets PR separately.

### What counts as “binary” (for this rule)

Treat as binary any non-text file (e.g., `*.png *.jpg *.gif *.zip *.7z *.mp4 *.avi *.wav *.mp3 *.ttf *.otf *.dll *.so *.dylib *.wasm`) and any file that `git diff` reports as binary.

### Codex workflow when changes include binaries

1. **Stage code only.**

   * Identify files staged for commit:
     `git diff --name-only --cached`
   * Identify new/modified binaries in the working copy:
     `git ls-files -vmo --exclude-standard` (untracked) + `git diff --name-only` (modified), then detect binaries via `git diff --numstat -- <path>` returning `-` `-`, or `file --mime`.
   * **Unstage binaries while leaving them in the working tree:**

     ```
     # for each binary path B
     git restore --staged -- B
     ```
   * Do **not** delete or ignore these files. They must remain in the working copy exactly where they belong.

2. **Create a ZIP artifact of binaries (repo-relative paths).**

   * Create a temp folder: `./_codex_artifacts/`
   * ZIP name: `_codex_artifacts/binaries-<branch>-<yyyymmdd-HHMMSS>.zip`
   * The ZIP must contain each binary at its **final repo-relative path** (so extracting at repo root puts it in place).

3. **Emit a manifest alongside the ZIP (inside the ZIP at repo root).**
   File: `BINARY_ASSETS.manifest.json` with entries:

   ```json
   {
     "schema": 1,
     "branch": "<branch>",
     "created_utc": "YYYY-MM-DDTHH:MM:SSZ",
     "files": [
       { "path": "tests/golden/phase1/fadeout_color.f120.png", "size": 123456, "sha256": "<hex>" },
       { "path": "tests/golden/phase1/fadeout_color.hash.txt",  "size": 64,     "sha256": "<hex>" }
     ]
   }
   ```

   * Compute SHA256 on file **bytes** exactly as they should be committed.

4. **Attach the ZIP to your assistant message** (not to the PR).

   * Provide the SHA256 of the ZIP itself in the message.

5. **Open a PR with code only.**

   * Commit message: `tests: expand deterministic harness (binaries packaged separately)`
   * PR title example: `tests: phase1 harness + wiring (binary assets attached out-of-band)`
   * **PR body must include a “Binary assets” section** (template below) listing:

     * the exact ZIP filename, size, and SHA256,
     * a table of the excluded binary files (path, size, sha256) from the manifest,
     * instructions that the maintainer will open a **companion PR** adding these binaries verbatim.

6. **Never alter .gitignore or hooks to block binaries.**
   We intentionally keep binaries allowed in the repository; this rule applies **only** to Codex-generated PRs.

### PR body template (Codex must paste this)

```
## Binary assets (excluded from this PR by policy)

To avoid Codex “create PR” failures on binary diffs, all binary changes for this branch
are packaged as a separate ZIP artifact:

ZIP: binaries-<branch>-<yyyymmdd-HHMMSS>.zip
Size: <bytes>
ZIP SHA256: <zip_sha256>

I have attached the ZIP in chat. Maintainer will open a separate PR that adds these
files verbatim.

| Path | Size | SHA256 |
|------|------:|-------:|
| tests/golden/phase1/fadeout_color.f120.png | 123456 | <hex> |
| tests/golden/phase1/fadeout_color.hash.txt | 64     | <hex> |

After applying the companion PR, run:
`ctest -R deterministic_render_test`
```

### Minimal helper commands Codex should run

* List candidate binaries (added/modified):

  ```
  git diff --name-only --diff-filter=AM > /tmp/candidates.txt
  while read -r p; do
    if git diff --numstat -- "$p" | grep -q '^-'; then
      echo "$p" >> /tmp/binaries.txt
    fi
  done < /tmp/candidates.txt
  ```
* Unstage them:

  ```
  xargs -a /tmp/binaries.txt -r git restore --staged --
  ```
* Build the ZIP at repo root (preserving paths):

  ```
  mkdir -p _codex_artifacts
  zip -q -r "_codex_artifacts/binaries-<branch>-<stamp>.zip" @/tmp/binaries.txt
  ```
* Manifest (pseudo): compute size and sha256 for each path and write `BINARY_ASSETS.manifest.json` into the ZIP root.

> Note: If tests require those binaries to pass in CI, mark them `GTEST_SKIP()` (or equivalent) when files are absent, and include a README note telling maintainers to apply the companion binary PR locally before running the full suite.

