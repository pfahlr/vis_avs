# Codex Intake

- Primary instructions are in `AGENTS.md`.
- Actionable jobs live under `codex/jobs/*.yaml`.
- Follow the schema documented in AGENTS.md (id, title, status, labels, depends_on, goal, steps, acceptance_criteria, test_hook, files_touched).
- Respect `codex/policies.md` (no binaries in PRs; warnings as errors).

## Environment bootstrapping (must do before configuring CMake)

- **Install build dependencies first.** Run the dev-environment helper before any `cmake` invocation so PortAudio and the other required packages are available inside the container. The top-level `CMakeLists.txt` will now invoke the helper automatically when packages are missing, but running it manually keeps the process explicit and allows skipping the automatic bootstrap when setting `AVS_SKIP_AUTO_DEPS=1`.

  ```bash
  ./run_setup_dev_environment.sh --platform ubuntu
  ```

  Use `--platform fedora` when working from a Fedora base image. Skipping this step causes `cmake -S . -B build` to fail with "PortAudio not found" because the headers and libraries are missing.
- After dependencies are installed, continue with the normal workflow (`cmake -S . -B build`, `cmake --build build`, `ctest`, …).
