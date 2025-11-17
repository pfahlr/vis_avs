# Contributing to vis_avs

Thank you for your interest in contributing to vis_avs! This document provides guidelines and best practices for contributing to this modern C++ reimplementation of Winamp's Advanced Visualization Studio.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [How to Contribute](#how-to-contribute)
4. [Development Workflow](#development-workflow)
5. [Coding Standards](#coding-standards)
6. [Commit Guidelines](#commit-guidelines)
7. [Pull Request Process](#pull-request-process)
8. [Testing Requirements](#testing-requirements)
9. [Documentation](#documentation)
10. [Community](#community)

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please:

- **Be respectful** - Treat all contributors with respect and professionalism
- **Be constructive** - Provide helpful feedback and suggestions
- **Be collaborative** - Work together to improve the project
- **Be patient** - Remember that everyone was a beginner once

If you experience or witness unacceptable behavior, please report it to the project maintainers.

## Getting Started

### Prerequisites

Before contributing, make sure you have:

1. **Read the documentation**:
   - [README.md](README.md) - Project overview
   - [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) - Development setup
   - [docs/CODEBASE_ARCHITECTURE.md](docs/CODEBASE_ARCHITECTURE.md) - Architecture overview

2. **Set up your development environment**:
   ```bash
   git clone https://github.com/pfahlr/vis_avs.git
   cd vis_avs
   ./run_setup_dev_environment.sh --platform ubuntu  # or fedora
   ```

3. **Build the project**:
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   cmake --build . -j$(nproc)
   ctest
   ```

4. **Verify tests pass**:
   ```bash
   ctest --output-on-failure
   ```

### Finding Something to Work On

**Browse available tasks**:
- Check [GitHub Issues](https://github.com/pfahlr/vis_avs/issues) for open bugs and feature requests
- Look at `codex/jobs/` for planned work (filter by `status: TODO`)
- Check issues labeled `good-first-issue` for beginner-friendly tasks
- Check issues labeled `help-wanted` for tasks needing contributors

**Ask questions**:
- Open a GitHub Discussion if you need clarification
- Comment on an issue before starting work to avoid duplicates

## How to Contribute

### Types of Contributions

We welcome all types of contributions:

#### 🐛 Bug Fixes
- Report bugs via GitHub Issues
- Include steps to reproduce
- Provide preset files if relevant
- Submit PR with fix and test

#### ✨ New Features
- Propose features via GitHub Issues or Discussions
- Discuss design before implementing
- Submit PR with implementation, tests, and documentation

#### 📚 Documentation
- Improve existing docs
- Add tutorials and guides
- Document effects and APIs
- Fix typos and clarify unclear sections

#### 🧪 Testing
- Add test coverage
- Report test failures
- Improve test infrastructure
- Create golden hash tests for effects

#### 🔧 Tooling
- Improve build system
- Add developer tools
- Enhance CI/CD pipeline

#### 🎨 Effects
- Port original AVS effects
- Optimize existing effects
- Create new modern effects

## Development Workflow

### 1. Create a Branch

Always work in a feature branch:

```bash
git checkout -b feature/descriptive-name
# or
git checkout -b fix/issue-123-brief-description
```

**Branch naming**:
- `feature/` - New features (e.g., `feature/add-blur-effect`)
- `fix/` - Bug fixes (e.g., `fix/crash-on-empty-preset`)
- `docs/` - Documentation (e.g., `docs/improve-readme`)
- `refactor/` - Code refactoring (e.g., `refactor/simplify-pipeline`)
- `test/` - Test improvements (e.g., `test/add-blur-coverage`)

### 2. Make Changes

Follow these principles:

- **Keep commits focused** - One logical change per commit
- **Write tests** - Add tests for new functionality
- **Update documentation** - Keep docs in sync with code
- **Run tests frequently** - `ctest` after changes
- **Format code** - Use `clang-format` before committing

### 3. Test Your Changes

```bash
# Build
cd build
cmake --build . -j$(nproc)

# Run all tests
ctest

# Run specific tests
ctest -R your_test_pattern

# Run with verbose output
ctest --output-on-failure

# Manual testing
./apps/avs-player/avs-player --preset ../tests/data/test.avs
```

### 4. Format Code

```bash
# Format changed files
clang-format -i path/to/file.cpp

# Format all source files
find libs apps -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### 5. Commit Changes

```bash
git add .
git commit -m "feat: add horizontal blur effect

- Implement box blur algorithm
- Add multi-threaded rendering support
- Include unit tests for various blur radii"
```

See [Commit Guidelines](#commit-guidelines) for details.

### 6. Push and Create PR

```bash
git push origin feature/your-branch-name
```

Then create a Pull Request on GitHub.

## Coding Standards

### C++ Style

We follow modern C++ best practices:

#### General Principles

- **C++17 standard** (C++20 features where beneficial)
- **RAII** - Use smart pointers, avoid raw `new`/`delete`
- **Const correctness** - Mark read-only data as `const`
- **Prefer STL** - Use `std::vector`, `std::array`, etc.
- **Avoid macros** - Use `constexpr` and templates instead
- **Namespace all code** - Use `namespace avs { }`

#### Naming Conventions

```cpp
// Classes: PascalCase
class EffectRegistry { };
class BlurEffect { };

// Functions/Methods: camelCase
void renderFrame();
bool loadPreset(const std::string& path);

// Variables: camelCase
int frameCount;
float intensity;

// Private members: camelCase with trailing underscore
class MyClass {
private:
  int value_;
  std::vector<int> buffer_;
};

// Constants: kPascalCase
constexpr int kMaxWidth = 4096;
const std::string kDefaultPreset = "default.avs";

// Enums: PascalCase for type, PascalCase for values
enum class BlendMode {
  Add,
  Multiply,
  Screen
};
```

#### File Organization

```cpp
// Header file: effect_name.h
#pragma once  // Preferred over include guards

#include <vector>  // System includes first
#include <string>

#include "avs/core/IEffect.hpp"  // Project includes second

namespace avs {
namespace effects {

class EffectName : public core::IEffect {
public:
  // Public API
  EffectName() = default;
  ~EffectName() override = default;

  // Interface implementation
  bool render(core::RenderContext& context) override;
  void setParams(const core::ParamBlock& params) override;

private:
  // Private members
  int param1_ = 0;
  float param2_ = 1.0f;

  // Private methods
  void helperMethod();
};

}  // namespace effects
}  // namespace avs
```

#### Code Formatting

We use `clang-format` with the project's `.clang-format` configuration:

```cpp
// Good: Proper formatting
bool MyEffect::render(RenderContext& context) {
  const int width = context.width;
  const int height = context.height;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      processPixel(x, y, context.framebuffer);
    }
  }

  return true;
}

// Bad: Inconsistent formatting
bool MyEffect::render(RenderContext &context){
    const int width=context.width;
    for(int y=0;y<height;++y){
      processPixel( x,y,context.framebuffer );
    }
    return true;}
```

**Auto-format before committing**:
```bash
clang-format -i your_file.cpp
```

### Performance Guidelines

- **Avoid allocations in render loop** - Reuse buffers
- **Prefer cache-friendly access** - Access memory linearly
- **Use const references** - Avoid unnecessary copies
- **Profile before optimizing** - Don't guess performance

#### Good Example
```cpp
bool BlurEffect::render(RenderContext& context) {
  // Reuse buffer (allocated once)
  if (tempBuffer_.size() != context.framebuffer.size()) {
    tempBuffer_.resize(context.framebuffer.size());
  }

  // Linear memory access (cache-friendly)
  const int totalPixels = context.width * context.height;
  for (int i = 0; i < totalPixels; ++i) {
    processPixel(i);
  }

  return true;
}
```

#### Bad Example
```cpp
bool BlurEffect::render(RenderContext& context) {
  // Allocates every frame (slow!)
  auto tempBuffer = new uint8_t[context.framebuffer.size()];

  // Random access (cache-unfriendly)
  for (int y = 0; y < context.height; ++y) {
    for (int x = 0; x < context.width; ++x) {
      processPixel(x, y);  // Random access
    }
  }

  delete[] tempBuffer;  // Manual memory management (error-prone)
  return true;
}
```

### Documentation Standards

#### Code Comments

```cpp
// Good: Explains "why" and non-obvious behavior
bool BlurEffect::render(RenderContext& context) {
  // Use thread-local storage to avoid contention between threads.
  // Each thread maintains its own prefix sum array for horizontal passes.
  thread_local std::vector<int> prefixSum;

  // Early exit if blur radius is zero (no-op)
  if (blurRadius_ == 0) {
    return true;
  }

  // ... implementation
}

// Bad: States the obvious
int x = 0;  // Set x to 0
++x;        // Increment x
```

#### API Documentation

Use Doxygen-style comments for public APIs:

```cpp
/**
 * @brief Applies a box blur to the framebuffer.
 *
 * Performs a separable box blur by computing horizontal and vertical
 * passes with the specified radius. Supports multi-threaded rendering
 * via smp_render().
 *
 * @param context Rendering context containing framebuffer and dimensions
 * @return true if rendering succeeded, false on error
 */
bool render(RenderContext& context) override;
```

## Commit Guidelines

We follow [Conventional Commits](https://www.conventionalcommits.org/) for clear, structured commit messages.

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

#### Type

| Type | Description | Example |
|------|-------------|---------|
| `feat` | New feature | `feat: add horizontal mirror effect` |
| `fix` | Bug fix | `fix: prevent crash on empty preset` |
| `docs` | Documentation | `docs: add effect development guide` |
| `style` | Formatting | `style: apply clang-format to blur.cpp` |
| `refactor` | Code restructuring | `refactor: simplify parameter parsing` |
| `perf` | Performance improvement | `perf: optimize blur using SIMD` |
| `test` | Add/update tests | `test: add coverage for mirror effect` |
| `build` | Build system changes | `build: add Pipewire detection` |
| `ci` | CI/CD changes | `ci: add sanitizer builds to workflow` |
| `chore` | Maintenance | `chore: update dependencies` |

#### Scope (optional)

Indicates the area affected:
- `core` - Core system
- `effects` - Effect implementations
- `preset` - Preset parser
- `audio` - Audio system
- `render` - Rendering
- `build` - Build system

#### Subject

- Use imperative mood ("add", not "added" or "adds")
- Don't capitalize first letter
- No period at the end
- Limit to 50 characters

#### Body (optional)

- Explain what and why, not how
- Wrap at 72 characters
- Use bullet points for multiple changes

#### Footer (optional)

- Reference issues: `Closes #123`
- Breaking changes: `BREAKING CHANGE: rename API`

### Examples

#### Simple Commit
```
feat: add vertical blur pass

Implement vertical blur using prefix sum arrays for O(n) complexity.
```

#### Detailed Commit
```
feat(effects): add multi-threaded blur rendering

- Implement row-based work distribution
- Add thread-local prefix sum arrays
- Support 1-4 threads with automatic detection
- 2x speedup on 4-core systems

Closes #42
```

#### Bug Fix
```
fix(preset): handle malformed JSON gracefully

Previously, malformed JSON would crash the parser. Now we catch
nlohmann::json exceptions and return a descriptive error message.

Fixes #157
```

#### Breaking Change
```
refactor(core)!: change IEffect::render signature

BREAKING CHANGE: IEffect::render() now returns bool instead of void.
Effects should return false on errors.

Migration:
  - Change `void render(...)` to `bool render(...)`
  - Return true on success, false on error
```

## Pull Request Process

### Before Submitting

Checklist:
- [ ] Code builds without errors: `cmake --build build`
- [ ] All tests pass: `ctest`
- [ ] Code is formatted: `clang-format -i` on changed files
- [ ] New functionality has tests
- [ ] Documentation is updated (if applicable)
- [ ] Commit messages follow guidelines
- [ ] Branch is up to date with main

### PR Title

Follow commit message format:
```
feat: add horizontal blur effect
fix: prevent crash on empty audio buffer
docs: improve effect development guide
```

### PR Description

Use this template:

```markdown
## Summary
Brief description of changes.

## Motivation
Why is this change needed? What problem does it solve?

## Changes
- List of changes made
- Organized by category

## Testing
How was this tested? Include:
- Unit tests added/modified
- Manual testing steps
- Performance impact (if applicable)

## Screenshots/Videos (if applicable)
Visual proof of changes.

## Related Issues
Closes #123
Related to #456

## Checklist
- [ ] Tests pass
- [ ] Code formatted
- [ ] Documentation updated
- [ ] No breaking changes (or documented)
```

### Review Process

1. **Automated checks** - CI must pass (build, tests, linting)
2. **Code review** - At least one maintainer approval required
3. **Address feedback** - Respond to review comments
4. **Squash commits** - If requested by reviewers
5. **Merge** - Maintainer will merge when approved

### After Merge

- Delete your feature branch
- Monitor for any issues
- Celebrate! 🎉

## Testing Requirements

### Unit Tests

Every new feature or bug fix should include tests:

```cpp
#include <gtest/gtest.h>
#include "avs/effects/your_effect.h"

TEST(YourEffect, DescriptiveName) {
  // Arrange
  YourEffect effect;
  avs::core::ParamBlock params;
  params.setInt("param1", 42);
  effect.setParams(params);

  // Act
  std::vector<uint8_t> pixels(320 * 240 * 4, 0);
  avs::core::RenderContext ctx = createTestContext(320, 240, pixels);
  bool result = effect.render(ctx);

  // Assert
  ASSERT_TRUE(result);
  EXPECT_EQ(expectedValue, actualValue);
}
```

### Golden Hash Tests

For rendering changes, update golden hashes:

```bash
cd build
./tools/gen_golden_md5 --preset ../tests/data/test.avs \
  --frames 10 --seed 1234 > ../tests/regression/data/expected.json

# Verify
ctest -R golden
```

### Manual Testing

Test with real presets:

```bash
./apps/avs-player/avs-player --preset path/to/preset.avs
```

## Documentation

### When to Update Documentation

Update documentation when you:
- Add new features
- Change public APIs
- Fix bugs (update troubleshooting)
- Improve build process
- Add new dependencies

### Documentation Locations

| Type | Location |
|------|----------|
| **User guide** | `README.md` |
| **Developer guide** | `docs/DEVELOPER_GUIDE.md` |
| **Architecture** | `docs/CODEBASE_ARCHITECTURE.md` |
| **Build system** | `docs/BUILD_SYSTEM_GUIDE.md` |
| **Effect development** | `docs/EFFECT_DEVELOPMENT_GUIDE.md` |
| **API docs** | Code comments (Doxygen) |
| **Effect specs** | `docs/effects/<effect-name>.md` |

### Documentation Style

- **Be concise** - Get to the point quickly
- **Use examples** - Show, don't just tell
- **Stay current** - Update docs with code changes
- **Check spelling** - Use a spell checker
- **Format consistently** - Follow existing doc style

## Community

### Getting Help

- **Documentation** - Check `docs/` first
- **GitHub Discussions** - Ask questions, share ideas
- **GitHub Issues** - Report bugs, request features
- **Code comments** - Many complex sections are documented

### Providing Help

- Answer questions in Discussions
- Review PRs
- Improve documentation
- Triage issues

### Recognition

Contributors are recognized in:
- Git commit history
- GitHub Contributors page
- Release notes
- Project acknowledgments

## Questions?

If you have questions about contributing:

1. Check [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)
2. Search existing GitHub Issues
3. Open a GitHub Discussion
4. Reach out to maintainers

Thank you for contributing to vis_avs! 🎵🎨✨
