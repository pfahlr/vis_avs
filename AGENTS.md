# AGENTS

## Setup Commands
- See `docs/README.md` for build and run instructions.

## Code Style
- Use C++17 with `-Wall -Wextra -Werror`.
- Format with `clang-format` (Google style, 100 columns).

## Dev Environment Tips
- Configure with CMake ≥ 3.22.
- Any modern C++17 compiler is supported.

## Testing
- From the build directory run:
  - `cmake --build .`
  - `ctest`

## PR Instructions
- Follow Conventional Commits.
- Ensure formatting and tests pass before opening a PR.
