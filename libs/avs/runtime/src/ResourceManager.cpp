#include "avs/runtime/ResourceManager.hpp"

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace {
std::filesystem::path normalizePath(const std::filesystem::path& input) {
  if (input.empty()) {
    return {};
  }
  std::filesystem::path normalized = input;
  std::error_code ec;
  if (!normalized.is_absolute()) {
    normalized = std::filesystem::absolute(normalized, ec);
    if (ec) {
      ec.clear();
      normalized = std::filesystem::path(input);
    }
  }
  std::filesystem::path canonical = std::filesystem::weakly_canonical(normalized, ec);
  if (!ec) {
    return canonical;
  }
  ec.clear();
  auto absolute = std::filesystem::absolute(normalized, ec);
  if (!ec) {
    return absolute;
  }
  return normalized;
}

std::vector<std::filesystem::path> buildDefaultPaths() {
  std::vector<std::filesystem::path> defaults;
  const char* env = std::getenv("AVS_RESOURCE_DIR");
  if (env != nullptr && env[0] != '\0') {
    defaults.push_back(normalizePath(env));
  }
#ifdef AVS_RUNTIME_BUILD_RESOURCES_DIR
  defaults.emplace_back(normalizePath(AVS_RUNTIME_BUILD_RESOURCES_DIR));
#endif
#ifdef AVS_RUNTIME_INSTALL_RESOURCES_DIR
  defaults.emplace_back(normalizePath(AVS_RUNTIME_INSTALL_RESOURCES_DIR));
#endif
  return defaults;
}
}  // namespace

namespace avs::runtime {

ResourceManager::ResourceManager(std::vector<std::filesystem::path> additionalPaths) {
  searchPaths_ = buildDefaultPaths();
  for (const auto& path : additionalPaths) {
    if (!path.empty()) {
      searchPaths_.push_back(normalizePath(path));
    }
  }
}

std::filesystem::path ResourceManager::resolve(const std::string& relativePath) const {
  std::filesystem::path rel(relativePath);
  if (rel.empty()) {
    throw std::runtime_error("Resource path must not be empty");
  }
  for (const auto& base : searchPaths_) {
    if (base.empty()) {
      continue;
    }
    std::filesystem::path candidate = base / rel;
    std::error_code existsError;
    if (std::filesystem::exists(candidate, existsError) && !existsError) {
      std::error_code canonicalError;
      std::filesystem::path canonical =
          std::filesystem::weakly_canonical(candidate, canonicalError);
      if (canonicalError) {
        canonicalError.clear();
        canonical = std::filesystem::absolute(candidate, canonicalError);
        if (canonicalError) {
          canonical = candidate;
        }
      }
      return canonical;
    }
  }

  std::ostringstream oss;
  oss << "Resource '" << relativePath << "' not found. Searched paths: ";
  for (size_t i = 0; i < searchPaths_.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << searchPaths_[i].string();
  }
  throw std::runtime_error(oss.str());
}

std::vector<std::filesystem::path> ResourceManager::search_paths() const { return searchPaths_; }

}  // namespace avs::runtime
