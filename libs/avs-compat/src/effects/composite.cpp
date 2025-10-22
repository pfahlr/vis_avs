#include <avs/effects.hpp>

namespace avs {

void CompositeEffect::addEffect(std::unique_ptr<Effect> effect) {
  if (effect) {
    children_.push_back(std::move(effect));
  }
}

void CompositeEffect::init(int w, int h) {
  width_ = w;
  height_ = h;
  for (auto& child : children_) {
    child->init(w, h);
  }
  for (auto& buffer : buffers_) {
    buffer.w = w;
    buffer.h = h;
    buffer.rgba.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
  }
}

void CompositeEffect::process(const Framebuffer& in, Framebuffer& out) {
  if (children_.empty()) {
    out = in;
    return;
  }

  if (width_ == 0 || height_ == 0) {
    width_ = in.w;
    height_ = in.h;
    for (auto& child : children_) {
      child->init(width_, height_);
    }
  }

  const Framebuffer* currentIn = &in;
  int bufferIndex = 0;
  for (size_t i = 0; i < children_.size(); ++i) {
    bool last = (i + 1 == children_.size());
    Framebuffer* target = nullptr;
    if (last) {
      out.w = width_;
      out.h = height_;
      out.rgba.resize(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4u);
      target = &out;
    } else {
      Framebuffer& buffer = buffers_[bufferIndex];
      buffer.w = width_;
      buffer.h = height_;
      buffer.rgba.resize(static_cast<size_t>(width_) * static_cast<size_t>(height_) * 4u);
      target = &buffer;
    }
    children_[i]->process(*currentIn, *target);
    currentIn = target;
    if (!last) {
      bufferIndex ^= 1;
    }
  }
}

}  // namespace avs

