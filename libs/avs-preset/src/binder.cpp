#include <avs/preset/binder.h>

#include <functional>

namespace avs::preset {
using namespace avs::effects;

static ParamList to_params(const IRNode& n) {
  ParamList out;
  out.reserve(n.params.size());
  for (const auto& p : n.params) {
    ParamValue v;
    v.name = p.name;
    v.kind = static_cast<ParamValue::Kind>(p.kind);
    v.f = p.f;
    v.i = p.i;
    v.b = p.b;
    v.s = p.s;
    out.push_back(std::move(v));
  }
  return out;
}

std::unique_ptr<Graph> bind_preset(const IRPreset& ir, const BindOptions& opt, Registry& reg) {
  auto g = std::make_unique<Graph>();
  BuildCtx ctx{.compat = opt.compat};

  std::function<void(const IRNode&, NodeHandle)> visit = [&](const IRNode& n, NodeHandle parent) {
    bool matched_legacy = false;
    auto fx = reg.make(n.token, to_params(n), ctx, &matched_legacy);
    if (!fx) {
      fx = make_unknown(n.token, to_params(n), ctx);
    }
    auto h = g->add_node(std::move(fx));
    if (parent.is_valid()) {
      g->connect(parent, h);
    }
    for (const auto& c : n.children) {
      visit(c, h);
    }
  };

  for (const auto& n : ir.root_nodes) {
    visit(n, {});
  }
  return g;
}

}  // namespace avs::preset
