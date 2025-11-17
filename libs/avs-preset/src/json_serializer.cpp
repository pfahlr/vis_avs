#include <avs/preset/json.h>

#include <json.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace avs::preset {

using json = nlohmann::json;

namespace {

// ============================================================================
// Serialization (IRPreset -> JSON)
// ============================================================================

std::string kindToString(IRParam::Kind kind) {
  switch (kind) {
    case IRParam::F32:
      return "float";
    case IRParam::I32:
      return "int";
    case IRParam::BOOL:
      return "bool";
    case IRParam::STR:
      return "string";
    default:
      return "unknown";
  }
}

json paramToJson(const IRParam& param) {
  json j;
  j["name"] = param.name;
  j["type"] = kindToString(param.kind);

  switch (param.kind) {
    case IRParam::F32:
      j["value"] = param.f;
      break;
    case IRParam::I32:
      j["value"] = param.i;
      break;
    case IRParam::BOOL:
      j["value"] = param.b;
      break;
    case IRParam::STR:
      j["value"] = param.s;
      break;
  }

  return j;
}

json nodeToJson(const IRNode& node) {
  json j;
  j["effect"] = node.token;

  if (node.order_index != 0) {
    j["order"] = node.order_index;
  }

  if (!node.params.empty()) {
    json params = json::array();
    for (const auto& param : node.params) {
      params.push_back(paramToJson(param));
    }
    j["params"] = params;
  }

  if (!node.children.empty()) {
    json children = json::array();
    for (const auto& child : node.children) {
      children.push_back(nodeToJson(child));
    }
    j["children"] = children;
  }

  return j;
}

// ============================================================================
// Deserialization (JSON -> IRPreset)
// ============================================================================

IRParam::Kind stringToKind(const std::string& type_str) {
  if (type_str == "float" || type_str == "f32") {
    return IRParam::F32;
  } else if (type_str == "int" || type_str == "i32") {
    return IRParam::I32;
  } else if (type_str == "bool" || type_str == "boolean") {
    return IRParam::BOOL;
  } else if (type_str == "string" || type_str == "str") {
    return IRParam::STR;
  } else {
    throw std::runtime_error("Unknown parameter type: " + type_str);
  }
}

IRParam jsonToParam(const json& j) {
  IRParam param;

  if (!j.contains("name") || !j["name"].is_string()) {
    throw std::runtime_error("Parameter must have a 'name' field");
  }
  param.name = j["name"];

  if (!j.contains("type") || !j["type"].is_string()) {
    throw std::runtime_error("Parameter '" + param.name + "' must have a 'type' field");
  }
  std::string type_str = j["type"];
  param.kind = stringToKind(type_str);

  if (!j.contains("value")) {
    throw std::runtime_error("Parameter '" + param.name + "' must have a 'value' field");
  }

  switch (param.kind) {
    case IRParam::F32:
      if (j["value"].is_number()) {
        param.f = j["value"];
      } else {
        throw std::runtime_error("Parameter '" + param.name +
                                  "' has type 'float' but value is not a number");
      }
      break;

    case IRParam::I32:
      if (j["value"].is_number_integer()) {
        param.i = j["value"];
      } else {
        throw std::runtime_error("Parameter '" + param.name +
                                  "' has type 'int' but value is not an integer");
      }
      break;

    case IRParam::BOOL:
      if (j["value"].is_boolean()) {
        param.b = j["value"];
      } else {
        throw std::runtime_error("Parameter '" + param.name +
                                  "' has type 'bool' but value is not a boolean");
      }
      break;

    case IRParam::STR:
      if (j["value"].is_string()) {
        param.s = j["value"];
      } else {
        throw std::runtime_error("Parameter '" + param.name +
                                  "' has type 'string' but value is not a string");
      }
      break;
  }

  return param;
}

IRNode jsonToNode(const json& j) {
  IRNode node;

  if (!j.contains("effect") || !j["effect"].is_string()) {
    throw std::runtime_error("Effect node must have an 'effect' field");
  }
  node.token = j["effect"];

  if (j.contains("order")) {
    if (j["order"].is_number_integer()) {
      node.order_index = j["order"];
    } else {
      throw std::runtime_error("Effect '" + node.token +
                                "' has 'order' field but it's not an integer");
    }
  }

  if (j.contains("params")) {
    if (!j["params"].is_array()) {
      throw std::runtime_error("Effect '" + node.token +
                                "' has 'params' field but it's not an array");
    }
    for (const auto& param_json : j["params"]) {
      node.params.push_back(jsonToParam(param_json));
    }
  }

  if (j.contains("children")) {
    if (!j["children"].is_array()) {
      throw std::runtime_error("Effect '" + node.token +
                                "' has 'children' field but it's not an array");
    }
    for (const auto& child_json : j["children"]) {
      node.children.push_back(jsonToNode(child_json));
    }
  }

  return node;
}

}  // namespace

// ============================================================================
// Public API
// ============================================================================

std::string serializeToJson(const IRPreset& preset, int indent) {
  json j;

  j["version"] = "1.0";
  j["compat"] = preset.compat;

  if (!preset.root_nodes.empty()) {
    json effects = json::array();
    for (const auto& node : preset.root_nodes) {
      effects.push_back(nodeToJson(node));
    }
    j["effects"] = effects;
  }

  return j.dump(indent);
}

IRPreset deserializeFromJson(std::string_view json_str) {
  IRPreset preset;

  try {
    json j = json::parse(json_str);

    if (j.contains("version")) {
      std::string version = j["version"];
      if (version != "1.0") {
        throw std::runtime_error("Unsupported JSON preset version: " + version);
      }
    }

    if (j.contains("compat")) {
      preset.compat = j["compat"];
    }

    if (j.contains("effects") && j["effects"].is_array()) {
      for (const auto& effect_json : j["effects"]) {
        preset.root_nodes.push_back(jsonToNode(effect_json));
      }
    }

  } catch (const json::parse_error& e) {
    throw std::runtime_error(std::string("JSON parse error: ") + e.what());
  } catch (const json::type_error& e) {
    throw std::runtime_error(std::string("JSON type error: ") + e.what());
  }

  return preset;
}

bool isJsonFormat(std::string_view data) {
  size_t i = 0;
  while (i < data.size() && std::isspace(static_cast<unsigned char>(data[i]))) {
    ++i;
  }
  return i < data.size() && data[i] == '{';
}

}  // namespace avs::preset
