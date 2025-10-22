#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <avs/runtime/ResourceManager.hpp>

namespace {
std::filesystem::path buildResourcesRoot() {
  return std::filesystem::path(BUILD_DIR) / "resources";
}

std::filesystem::path expectedPalettePath() {
  return buildResourcesRoot() / "palettes" / "test_gradient.pal";
}

class ScopedEnvVar {
 public:
  ScopedEnvVar(const std::string &name, const std::string &value) : name_(name) {
#ifdef _WIN32
    char *existing = nullptr;
    size_t len = 0;
    if (_dupenv_s(&existing, &len, name.c_str()) == 0 && existing != nullptr) {
      has_old_value_ = true;
      old_value_ = existing;
      free(existing);
    }
    _putenv_s(name.c_str(), value.c_str());
#else
    const char *existing = std::getenv(name.c_str());
    if (existing != nullptr) {
      has_old_value_ = true;
      old_value_ = existing;
    }
    ::setenv(name.c_str(), value.c_str(), 1);
#endif
  }

  ~ScopedEnvVar() {
#ifdef _WIN32
    if (has_old_value_) {
      _putenv_s(name_.c_str(), old_value_.c_str());
    } else {
      _putenv_s(name_.c_str(), "");
    }
#else
    if (has_old_value_) {
      ::setenv(name_.c_str(), old_value_.c_str(), 1);
    } else {
      ::unsetenv(name_.c_str());
    }
#endif
  }

  ScopedEnvVar(const ScopedEnvVar &) = delete;
  ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

 private:
  std::string name_;
  std::string old_value_;
  bool has_old_value_ = false;
};
}  // namespace

TEST(ResourceManagerTest, ResolvesPaletteFromBuildTree) {
  const auto palette = expectedPalettePath();
  ASSERT_TRUE(std::filesystem::exists(palette))
      << "Test palette missing; ensure resources copied to build tree";

  avs::runtime::ResourceManager manager;
  const auto resolved = manager.resolve("palettes/test_gradient.pal");

  EXPECT_TRUE(std::filesystem::exists(resolved));
  EXPECT_TRUE(resolved.is_absolute());
  EXPECT_EQ(std::filesystem::weakly_canonical(palette),
            std::filesystem::weakly_canonical(resolved));
}

TEST(ResourceManagerTest, PrefersEnvironmentDirectory) {
  const auto palette = expectedPalettePath();
  ASSERT_TRUE(std::filesystem::exists(palette));

  const auto temp_root = std::filesystem::path(BUILD_DIR) / "tests" / "tmp" / "resources_env";
  const auto env_palette = temp_root / "palettes" / "test_gradient.pal";
  std::filesystem::create_directories(env_palette.parent_path());
  std::filesystem::copy_file(palette, env_palette,
                             std::filesystem::copy_options::overwrite_existing);

  ScopedEnvVar scoped("AVS_RESOURCE_DIR", temp_root.string());

  avs::runtime::ResourceManager manager;
  const auto resolved = manager.resolve("palettes/test_gradient.pal");

  EXPECT_EQ(std::filesystem::weakly_canonical(env_palette),
            std::filesystem::weakly_canonical(resolved));
}

TEST(ResourceManagerTest, ThrowsHelpfulErrorWhenMissing) {
  ScopedEnvVar scoped("AVS_RESOURCE_DIR", "");
  avs::runtime::ResourceManager manager;
  const std::string rel = "palettes/does_not_exist.pal";

  try {
    (void)manager.resolve(rel);
    FAIL() << "Expected resolve to throw";
  } catch (const std::runtime_error &err) {
    const auto message = std::string(err.what());
    EXPECT_NE(message.find(rel), std::string::npos);
    for (const auto &path : manager.search_paths()) {
      EXPECT_NE(message.find(path.string()), std::string::npos)
          << "Error message should mention search path: " << path;
    }
  }
}
