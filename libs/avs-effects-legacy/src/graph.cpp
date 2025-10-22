#include <avs/effects/graph.h>

#include <string>
#include <utility>

namespace avs::effects {

namespace {
class UnknownEffect : public IEffect {
 public:
  explicit UnknownEffect(std::string id) : id_(std::move(id)) {}
  const char* id() const override { return id_.c_str(); }

 private:
  std::string id_;
};
}  // namespace

NodeHandle Graph::add_node(std::unique_ptr<IEffect> fx) {
  NodeHandle handle{static_cast<int>(nodes_.size())};
  nodes_.push_back(std::move(fx));
  edges_.emplace_back();
  return handle;
}

void Graph::connect(NodeHandle parent, NodeHandle child) {
  if (!parent.is_valid() || !child.is_valid()) {
    return;
  }
  const auto parentIdx = static_cast<std::size_t>(parent.idx);
  const auto childIdx = static_cast<std::size_t>(child.idx);
  if (parentIdx >= edges_.size() || childIdx >= nodes_.size()) {
    return;
  }
  edges_[parentIdx].push_back(child.idx);
}

std::unique_ptr<IEffect> make_unknown(const std::string& token, const ParamList&, const BuildCtx&) {
  return std::make_unique<UnknownEffect>(token);
}

}  // namespace avs::effects
