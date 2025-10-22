#pragma once
#include <memory>
#include <string>
#include <vector>

#include "api.h"

namespace avs::effects {

struct NodeHandle {
  int idx = -1;
  bool is_valid() const { return idx >= 0; }
};

class Graph {
 public:
  NodeHandle add_node(std::unique_ptr<IEffect> fx);
  void connect(NodeHandle parent, NodeHandle child);
  size_t size() const { return nodes_.size(); }

  const std::vector<std::unique_ptr<IEffect>>& nodes() const { return nodes_; }
  const std::vector<std::vector<int>>& edges() const { return edges_; }

 private:
  std::vector<std::unique_ptr<IEffect>> nodes_;
  std::vector<std::vector<int>> edges_;
};

std::unique_ptr<IEffect> make_unknown(const std::string& token, const ParamList& params,
                                      const BuildCtx& ctx);

}  // namespace avs::effects
