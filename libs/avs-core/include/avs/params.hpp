#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <optional>
#include "core.hpp"

namespace avs {

// Parameter schema to describe effect UI & serialization.
// Values can be read at runtime for dynamic control (e.g., from JSON/YAML).

struct OptionItem {
  std::string id;     // e.g., "Left"
  std::string label;  // display
};

using ParamValue = std::variant<
  bool, int, float, std::string, ColorRGBA8
>;

enum class ParamKind : uint8_t {
  Bool, Int, Float, Color, String, Select, Resource, List
};

struct Param {
  std::string name;
  ParamKind kind{ParamKind::Float};
  ParamValue value{};
  // Optional constraints
  std::optional<int> i_min, i_max;
  std::optional<float> f_min, f_max;
  std::vector<OptionItem> options; // for Select
};

// A list-param can hold children; keep a flat array keyed by prefix paths, or
// use nested structures at the serialization layer. For headers, keep it simple.
struct ParamList {
  std::vector<Param> items;
};

} // namespace avs
