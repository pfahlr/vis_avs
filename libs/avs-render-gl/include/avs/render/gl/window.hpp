#pragma once

#include <cstdint>
#include <memory>
#include <utility>

namespace avs {

class Window {
 public:
  Window(int w, int h, const char* title);
  ~Window();

  bool poll();
  std::pair<int, int> size() const;
  void blit(const std::uint8_t* rgba, int width, int height);
  bool keyPressed(int key);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace avs
