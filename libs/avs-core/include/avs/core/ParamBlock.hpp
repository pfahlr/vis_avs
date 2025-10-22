#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace avs::core {

/**
 * @brief Typed container for effect parameters.
 */
class ParamBlock {
 public:
  using Value = std::variant<int, float, bool, std::string>;

  void setInt(const std::string& key, int value) { values_[key] = value; }
  void setFloat(const std::string& key, float value) { values_[key] = value; }
  void setBool(const std::string& key, bool value) { values_[key] = value; }
  void setString(const std::string& key, std::string value) {
    values_[key] = std::move(value);
  }

  [[nodiscard]] bool contains(const std::string& key) const {
    return values_.find(key) != values_.end();
  }

  [[nodiscard]] int getInt(const std::string& key, int defaultValue = 0) const {
    if (auto value = getValue<int>(key)) {
      return *value;
    }
    if (auto valueFloat = getValue<float>(key)) {
      return static_cast<int>(*valueFloat);
    }
    return defaultValue;
  }

  [[nodiscard]] float getFloat(const std::string& key, float defaultValue = 0.0f) const {
    if (auto value = getValue<float>(key)) {
      return *value;
    }
    if (auto valueInt = getValue<int>(key)) {
      return static_cast<float>(*valueInt);
    }
    return defaultValue;
  }

  [[nodiscard]] bool getBool(const std::string& key, bool defaultValue = false) const {
    if (auto value = getValue<bool>(key)) {
      return *value;
    }
    return defaultValue;
  }

  [[nodiscard]] std::string getString(const std::string& key,
                                      std::string defaultValue = {}) const {
    if (auto value = getValue<std::string>(key)) {
      return *value;
    }
    return defaultValue;
  }

 private:
  template <typename T>
  [[nodiscard]] std::optional<T> getValue(const std::string& key) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
      return std::nullopt;
    }
    if (auto value = std::get_if<T>(&it->second)) {
      return *value;
    }
    return std::nullopt;
  }

  std::unordered_map<std::string, Value> values_;
};

}  // namespace avs::core
