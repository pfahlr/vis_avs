#include "avs/effects_misc.hpp"

#include "avs/core.hpp"
#include "avs/params.hpp"
#include "avs/runtime/framebuffers.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace avs {
namespace {

std::vector<OptionItem> bufferSlotOptions() {
  std::vector<OptionItem> options;
  const char labels[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
  for (int i = 0; i < 8; ++i) {
    std::string id(1, labels[i]);
    options.push_back(OptionItem{id, id});
  }
  return options;
}

runtime::BufferSlot slotFromIndex(int index) {
  index = std::clamp(index, 0, 7);
  return static_cast<runtime::BufferSlot>(index);
}

int slotToIndex(runtime::BufferSlot slot) {
  return static_cast<int>(static_cast<std::size_t>(slot));
}

std::optional<runtime::BufferSlot> slotFromValue(const ParamValue& value) {
  if (std::holds_alternative<int>(value)) {
    return slotFromIndex(std::get<int>(value));
  }
  if (std::holds_alternative<std::string>(value)) {
    const std::string& text = std::get<std::string>(value);
    if (!text.empty()) {
      char c = static_cast<char>(std::toupper(static_cast<unsigned char>(text.front())));
      if (c >= 'A' && c <= 'H') {
        return slotFromIndex(c - 'A');
      }
    }
  }
  if (std::holds_alternative<float>(value)) {
    int idx = static_cast<int>(std::lround(std::get<float>(value)));
    return slotFromIndex(idx);
  }
  return std::nullopt;
}

class EffectListConfigParser {
 public:
  explicit EffectListConfigParser(std::string_view text) : text_(text) {}

  bool parse(std::vector<EffectListEffect::ConfigNode>& out) {
    out.clear();
    skipWhitespace();
    if (peek() == '[') {
      get();
      if (!parseArray(out)) return false;
      skipWhitespace();
      return skipTrailing();
    }
    EffectListEffect::ConfigNode node;
    if (!parseObject(node)) return false;
    out.push_back(std::move(node));
    return skipTrailing();
  }

 private:
  bool parseArray(std::vector<EffectListEffect::ConfigNode>& out) {
    skipWhitespace();
    if (peek() == ']') {
      get();
      return true;
    }
    while (pos_ < text_.size()) {
      EffectListEffect::ConfigNode node;
      if (!parseObject(node)) return false;
      out.push_back(std::move(node));
      skipWhitespace();
      if (peek() == ',') {
        get();
        skipWhitespace();
        continue;
      }
      if (peek() == ']') {
        get();
        return true;
      }
      return false;
    }
    return false;
  }

  bool parseObject(EffectListEffect::ConfigNode& node) {
    if (!consume('{')) return false;
    skipWhitespace();
    if (peek() == '}') {
      get();
      return !node.id.empty();
    }
    while (pos_ < text_.size()) {
      std::string key;
      if (!parseString(key)) return false;
      skipWhitespace();
      if (!consume(':')) return false;
      skipWhitespace();
      if (key == "effect") {
        std::string value;
        if (!parseString(value)) return false;
        node.id = value;
      } else if (key == "children") {
        if (!consume('[')) return false;
        if (!parseArray(node.children)) return false;
      } else {
        if (!skipValue()) return false;
      }
      skipWhitespace();
      if (peek() == ',') {
        get();
        skipWhitespace();
        continue;
      }
      break;
    }
    if (!consume('}')) return false;
    return !node.id.empty();
  }

  bool skipValue() {
    skipWhitespace();
    char c = peek();
    if (c == '"') {
      std::string tmp;
      return parseString(tmp);
    }
    if (c == '{') {
      EffectListEffect::ConfigNode tmp;
      return parseObject(tmp);
    }
    if (c == '[') {
      get();
      std::vector<EffectListEffect::ConfigNode> tmp;
      if (!parseArray(tmp)) return false;
      return true;
    }
    std::size_t start = pos_;
    while (pos_ < text_.size()) {
      c = peek();
      if (c == ',' || c == '}' || c == ']') break;
      ++pos_;
    }
    return pos_ > start;
  }

  bool parseString(std::string& out) {
    if (!consume('"')) return false;
    out.clear();
    while (pos_ < text_.size()) {
      char c = get();
      if (c == '"') return true;
      if (c == '\\') {
        if (pos_ >= text_.size()) return false;
        char esc = get();
        switch (esc) {
          case '"':
          case '\\':
          case '/':
            out.push_back(esc);
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          default:
            return false;
        }
      } else {
        out.push_back(c);
      }
    }
    return false;
  }

  void skipWhitespace() {
    while (pos_ < text_.size()) {
      char c = text_[pos_];
      if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
        ++pos_;
      } else {
        break;
      }
    }
  }

  bool skipTrailing() {
    skipWhitespace();
    return pos_ >= text_.size();
  }

  char peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }
  char get() { return pos_ < text_.size() ? text_[pos_++] : '\0'; }
  bool consume(char expected) {
    if (peek() != expected) return false;
    ++pos_;
    return true;
  }

  std::string_view text_;
  std::size_t pos_{0};
};

}  // namespace

// Effect List
void EffectListEffect::setFactory(Factory factory) {
  factory_ = std::move(factory);
  if (!configTree_.empty()) {
    rebuildChildren();
  }
}

void EffectListEffect::init(const InitContext& ctx) {
  initContext_ = ctx;
  initialized_ = true;
  for (auto& child : children_) {
    child->init(ctx);
  }
}

void EffectListEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  for (auto& child : children_) {
    child->process(ctx, dst);
  }
}

std::vector<Param> EffectListEffect::parameters() const {
  return {Param{"config", ParamKind::String, config_}};
}

void EffectListEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name != "config" || !std::holds_alternative<std::string>(value)) {
    return;
  }
  config_ = std::get<std::string>(value);
  EffectListConfigParser parser(config_);
  std::vector<ConfigNode> parsed;
  if (!config_.empty() && !parser.parse(parsed)) {
    configTree_.clear();
    children_.clear();
    return;
  }
  configTree_ = std::move(parsed);
  rebuildChildren();
}

void EffectListEffect::rebuildChildren() {
  if (!factory_) {
    children_.clear();
    return;
  }
  std::vector<std::unique_ptr<IEffect>> rebuilt;
  rebuilt.reserve(configTree_.size());
  for (const auto& node : configTree_) {
    if (node.id.empty()) continue;
    auto child = factory_(node.id);
    if (!child) continue;
    if (auto* listChild = dynamic_cast<EffectListEffect*>(child.get())) {
      listChild->setFactory(factory_);
      listChild->configTree_ = node.children;
      listChild->config_.clear();
      listChild->rebuildChildren();
    }
    rebuilt.push_back(std::move(child));
  }
  children_ = std::move(rebuilt);
  if (initialized_) {
    for (auto& child : children_) {
      child->init(initContext_);
    }
  }
}

// Global Variables (stub)
void GlobalVariablesEffect::init(const InitContext&) {}
std::vector<Param> GlobalVariablesEffect::parameters() const { return {}; }
void GlobalVariablesEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> SaveBufferEffect::parameters() const {
  Param slot{"slot", ParamKind::Select, slotToIndex(slot_)};
  slot.options = bufferSlotOptions();
  return {slot};
}

void SaveBufferEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name != "slot") return;
  if (auto slot = slotFromValue(value)) {
    slot_ = *slot;
  }
}

void SaveBufferEffect::process(const ProcessContext& ctx, FrameBufferView&) {
  if (!ctx.fb.registers) return;
  ctx.fb.registers->save(slot_);
}

std::vector<Param> RestoreBufferEffect::parameters() const {
  Param slot{"slot", ParamKind::Select, slotToIndex(slot_)};
  slot.options = bufferSlotOptions();
  return {slot};
}

void RestoreBufferEffect::set_parameter(std::string_view name, const ParamValue& value) {
  if (name != "slot") return;
  if (auto slot = slotFromValue(value)) {
    slot_ = *slot;
  }
}

void RestoreBufferEffect::process(const ProcessContext& ctx, FrameBufferView&) {
  if (!ctx.fb.registers) return;
  ctx.fb.registers->restore(slot_);
}

std::vector<Param> OnBeatClearEffect::parameters() const {
  return {{"amount", ParamKind::Float, 1.0f, 0.0f, 1.0f}};
}

void OnBeatClearEffect::process(const ProcessContext& ctx, FrameBufferView& dst) {
  if (!ctx.audio.beat) return;
  for (int y = 0; y < dst.height; ++y) {
    std::uint8_t* row = dst.data + y * dst.stride;
    std::fill(row, row + dst.width * 4, 0);
  }
}

std::vector<Param> ClearScreenEffect::parameters() const {
  return {{"color", ParamKind::Color, ColorRGBA8{0, 0, 0, 255}}};
}

void ClearScreenEffect::process(const ProcessContext&, FrameBufferView& dst) {
  for (int y = 0; y < dst.height; ++y) {
    std::uint8_t* row = dst.data + y * dst.stride;
    for (int x = 0; x < dst.width; ++x) {
      row[x * 4 + 0] = 0;
      row[x * 4 + 1] = 0;
      row[x * 4 + 2] = 0;
      row[x * 4 + 3] = 255;
    }
  }
}

}  // namespace avs
