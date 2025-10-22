#pragma once

#include <string>

#include <avs/core/IEffect.hpp>

namespace avs::effects::misc {

/**
 * @brief Effect that stores preset comments without altering the framebuffer.
 */
class Comment : public avs::core::IEffect {
 public:
  Comment() = default;
  ~Comment() override = default;

  bool render(avs::core::RenderContext& context) override;
  void setParams(const avs::core::ParamBlock& params) override;

  [[nodiscard]] const std::string& text() const noexcept { return comment_; }

 private:
  std::string comment_;
};

}  // namespace avs::effects::misc
