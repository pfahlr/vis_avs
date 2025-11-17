#include <avs/effects/api.h>
#include <avs/effects/registry.h>
#include <avs/effects/trans/effect_color_modifier.h>

#include <avs/core/ParamBlock.hpp>
#include <memory>
#include <string>
#include <utility>

namespace avs::effects::legacy {
namespace {
class ColorModifierNode : public IEffect {
 public:
  ColorModifierNode() : impl_(std::make_unique<avs::effects::trans::ColorModifier>()) {}

  const char* id() const override { return "trans/color_modifier"; }

  avs::effects::trans::ColorModifier& effect() { return *impl_; }

 private:
  std::unique_ptr<avs::effects::trans::ColorModifier> impl_;
};

avs::core::ParamBlock to_param_block(const ParamList& params) {
  avs::core::ParamBlock block;
  for (const auto& param : params) {
    if (param.name.empty()) {
      continue;
    }
    switch (param.kind) {
      case ParamValue::F32:
        block.setFloat(param.name, param.f);
        break;
      case ParamValue::I32:
        block.setInt(param.name, param.i);
        break;
      case ParamValue::BOOL:
        block.setBool(param.name, param.b);
        break;
      case ParamValue::STR:
        block.setString(param.name, param.s);
        break;
    }
  }
  return block;
}

std::unique_ptr<IEffect> make_color_modifier(const ParamList& params, const BuildCtx&) {
  auto node = std::make_unique<ColorModifierNode>();
  auto block = to_param_block(params);
  node->effect().setParams(block);
  return node;
}

Descriptor color_modifier_desc() {
  return Descriptor{.id = "trans/color_modifier",
                    .legacy_tokens = {"Trans / Color Modifier", "trans/color modifier"},
                    .factory = make_color_modifier};
}

}  // namespace

void register_color_modifier(Registry& r) { r.add(color_modifier_desc()); }

}  // namespace avs::effects::legacy
