#pragma once
#include <string>
#include <vector>

namespace avs::preset {

struct IRParam {
  std::string name;
  enum Kind { F32, I32, BOOL, STR } kind = F32;
  float f = 0.f;
  int i = 0;
  bool b = false;
  std::string s;
};

struct IRNode {
  std::string token;
  std::vector<IRParam> params;
  std::vector<IRNode> children;
  int order_index = 0;
};

struct IRPreset {
  std::vector<IRNode> root_nodes;
  std::string compat = "strict";
};

}  // namespace avs::preset
