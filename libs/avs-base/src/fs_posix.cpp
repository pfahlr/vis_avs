#include <sys/inotify.h>
#include <unistd.h>

#include <filesystem>

#include <avs/fs.hpp>

namespace avs {

FileWatcher::FileWatcher(const std::filesystem::path& path) : path_(path) {
#ifdef __linux__
  fd_ = inotify_init1(IN_NONBLOCK);
  if (fd_ >= 0) {
    wd_ = inotify_add_watch(fd_, path_.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_MODIFY);
    if (wd_ < 0) {
      close(fd_);
      fd_ = -1;
    }
  }
#endif
  if (fd_ < 0) {
    if (std::filesystem::exists(path_)) last_ = std::filesystem::last_write_time(path_);
  }
}

FileWatcher::~FileWatcher() {
#ifdef __linux__
  if (wd_ >= 0) inotify_rm_watch(fd_, wd_);
  if (fd_ >= 0) close(fd_);
#endif
}

bool FileWatcher::poll() {
#ifdef __linux__
  if (fd_ >= 0) {
    char buf[sizeof(inotify_event) * 4];
    int len = read(fd_, buf, sizeof(buf));
    if (len > 0) return true;
  }
#endif
  auto t = std::filesystem::exists(path_) ? std::filesystem::last_write_time(path_)
                                          : std::filesystem::file_time_type::min();
  if (t != last_) {
    last_ = t;
    return true;
  }
  return false;
}

}  // namespace avs
