#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "core.hpp"

namespace avs {

// Parameter schema to describe effect UI & serialization.
// Values can be read at runtime for dynamic control (e.g., from JSON/YAML).

struct OptionItem {
  std::string id;     // e.g., "Left"
  std::string label;  // display
};

using ParamValue = std::variant<bool, int, float, std::string, ColorRGBA8>;

enum class ParamKind : uint8_t { Bool, Int, Float, Color, String, Select, Resource, List };

struct Param {
  std::string name;
  ParamKind kind{ParamKind::Float};
  ParamValue value{};
  // Optional constraints
  std::optional<int> i_min, i_max;
  std::optional<float> f_min, f_max;
  std::vector<OptionItem> options;  // for Select

  Param() = default;
  Param(std::string name_, ParamKind kind_, ParamValue value_,
        std::optional<int> i_min_ = std::nullopt, std::optional<int> i_max_ = std::nullopt,
        std::optional<float> f_min_ = std::nullopt, std::optional<float> f_max_ = std::nullopt,
        std::vector<OptionItem> options_ = {})
      : name(std::move(name_)),
        kind(kind_),
        value(std::move(value_)),
        i_min(std::move(i_min_)),
        i_max(std::move(i_max_)),
        f_min(std::move(f_min_)),
        f_max(std::move(f_max_)),
        options(std::move(options_)) {}
};

// A list-param can hold children; keep a flat array keyed by prefix paths, or
// use nested structures at the serialization layer. For headers, keep it simple.
struct ParamList {
  std::vector<Param> items;
};

}  // namespace avs
