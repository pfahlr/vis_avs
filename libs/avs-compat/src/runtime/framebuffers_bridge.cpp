#include "runtime/framebuffers.h"

#include <algorithm>
#include <cstring>

namespace avs::runtime {
namespace {

FrameBufferView toCoreView(const FrameView& view) {
  return FrameBufferView{view.data, view.width, view.height, view.stride};
}

}  // namespace

FrameBuffers makeFrameBuffers(Framebuffers& buffers) {
  FrameBuffers views{};
  refreshFrameBuffers(buffers, views);
  return views;
}

void refreshFrameBuffers(Framebuffers& buffers, FrameBuffers& views) {
  FrameView cur = buffers.currentView();
  FrameView prev = buffers.previousView();
  views.current = toCoreView(cur);
  views.previous = toCoreView(prev);
  views.registers = &buffers;
}

void copyFrame(const FrameView& src, const FrameBufferView& dst) {
  if (!src.data || !dst.data) return;
  const int width = std::min(src.width, dst.width);
  const int height = std::min(src.height, dst.height);
  if (width <= 0 || height <= 0) return;
  const int channels = 4;
  const int rowBytes = width * channels;
  for (int y = 0; y < height; ++y) {
    const std::uint8_t* s = src.data + y * src.stride;
    std::uint8_t* d = dst.data + y * dst.stride;
    std::memcpy(d, s, static_cast<std::size_t>(rowBytes));
  }
}

void copyFrame(const FrameBufferView& src, const FrameView& dst) {
  if (!src.data || !dst.data) return;
  const int width = std::min(src.width, dst.width);
  const int height = std::min(src.height, dst.height);
  if (width <= 0 || height <= 0) return;
  const int channels = 4;
  const int rowBytes = width * channels;
  for (int y = 0; y < height; ++y) {
    const std::uint8_t* s = src.data + y * src.stride;
    std::uint8_t* d = dst.data + y * dst.stride;
    std::memcpy(d, s, static_cast<std::size_t>(rowBytes));
  }
}

}  // namespace avs::runtime
