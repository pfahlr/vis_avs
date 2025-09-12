#pragma once

#include <cstdint>
#include <utility>

namespace avs {

class Window {
 public:
  Window(int w, int h, const char* title);
  ~Window();

  bool poll();
  std::pair<int, int> size() const;
  void blit(const std::uint8_t* rgba, int width, int height);

 private:
  struct Impl;
  Impl* impl_;
};

}  // namespace avs
