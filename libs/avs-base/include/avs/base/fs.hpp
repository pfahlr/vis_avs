#pragma once

#include <filesystem>

namespace avs {

class FileWatcher {
 public:
  explicit FileWatcher(const std::filesystem::path& path);
  ~FileWatcher();

  // Returns true if the file changed since the last call.
  bool poll();

 private:
  std::filesystem::path path_;
  std::filesystem::file_time_type last_{};
#ifdef __linux__
  int fd_ = -1;
  int wd_ = -1;
#endif
};

}  // namespace avs
