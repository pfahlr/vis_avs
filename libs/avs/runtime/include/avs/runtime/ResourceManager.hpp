#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace avs::runtime {

/**
 * @brief Resolves asset locations for runtime resources.
 *
 * The manager inspects a prioritized list of base directories to locate
 * resource files required by the application. The default search order checks
 * the AVS_RESOURCE_DIR environment variable, the build-tree resource directory
 * and finally the install-tree resource directory.
 */
class ResourceManager {
 public:
  explicit ResourceManager(std::vector<std::filesystem::path> additionalPaths = {});

  /**
   * @brief Resolves a resource path relative to the configured search roots.
   * @param relativePath Resource relative path (POSIX-style separators).
   * @return Absolute filesystem path to the located resource.
   * @throws std::runtime_error if the resource cannot be found in any search root.
   */
  [[nodiscard]] std::filesystem::path resolve(const std::string& relativePath) const;

  /**
   * @brief Returns the ordered search paths inspected during resolution.
   */
  [[nodiscard]] std::vector<std::filesystem::path> search_paths() const;

 private:
  std::vector<std::filesystem::path> searchPaths_;
};

}  // namespace avs::runtime
