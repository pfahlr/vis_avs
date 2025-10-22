#include "effects/misc/effect_comment.h"

#include <array>
#include <string>
#include <string_view>

namespace avs::effects::misc {

namespace {

std::string extractComment(const avs::core::ParamBlock& params, const std::string& current) {
  static constexpr std::array<std::string_view, 4> kKeys{"comment", "text", "message", "msg"};
  for (std::string_view keyView : kKeys) {
    const std::string key{keyView};
    const std::string value = params.getString(key, current);
    if (params.contains(key) || value != current) {
      return value;
    }
  }
  return current;
}

}  // namespace

bool Comment::render(avs::core::RenderContext& context) {
  (void)context;
  return true;
}

void Comment::setParams(const avs::core::ParamBlock& params) {
  comment_ = extractComment(params, comment_);
}

}  // namespace avs::effects::misc
