#include <avs/effects/core/effect_scripted.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr int kInstructionBudgetBytes = 400000;
constexpr int kFontHeight = 7;
constexpr int kFontMaxWidth = 5;
constexpr int kGlyphSpacing = 1;

struct Glyph {
  int width = 0;
  std::array<std::uint8_t, kFontMaxWidth> columns{};
};

Glyph makeGlyph(std::initializer_list<std::string_view> rows) {
  Glyph glyph{};
  int rowIndex = 0;
  for (std::string_view row : rows) {
    glyph.width = std::min<int>(kFontMaxWidth, static_cast<int>(row.size()));
    for (int col = 0; col < glyph.width; ++col) {
      if (row[col] != ' ') {
        glyph.columns[static_cast<std::size_t>(col)] |= static_cast<std::uint8_t>(1u << rowIndex);
      }
    }
    ++rowIndex;
    if (rowIndex >= kFontHeight) {
      break;
    }
  }
  return glyph;
}

const std::unordered_map<char, Glyph>& fontMap() {
  static const std::unordered_map<char, Glyph> kFont = [] {
    std::unordered_map<char, Glyph> map;
    map.emplace('0', makeGlyph({" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}));
    map.emplace('1', makeGlyph({"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}));
    map.emplace('2', makeGlyph({" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}));
    map.emplace('3', makeGlyph({" ### ", "#   #", "    #", " ### ", "    #", "#   #", " ### "}));
    map.emplace('4', makeGlyph({"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}));
    map.emplace('5', makeGlyph({"#####", "#    ", "#    ", "#### ", "    #", "#   #", " ### "}));
    map.emplace('6', makeGlyph({" ### ", "#   #", "#    ", "#### ", "#   #", "#   #", " ### "}));
    map.emplace('7', makeGlyph({"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}));
    map.emplace('8', makeGlyph({" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}));
    map.emplace('9', makeGlyph({" ### ", "#   #", "#   #", " ####", "    #", "#   #", " ### "}));

    map.emplace('A', makeGlyph({" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}));
    map.emplace('B', makeGlyph({"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}));
    map.emplace('C', makeGlyph({" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}));
    map.emplace('D', makeGlyph({"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}));
    map.emplace('E', makeGlyph({"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}));
    map.emplace('F', makeGlyph({"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}));
    map.emplace('G', makeGlyph({" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ####"}));
    map.emplace('H', makeGlyph({"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}));
    map.emplace('I', makeGlyph({" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}));
    map.emplace('L', makeGlyph({"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}));
    map.emplace('M', makeGlyph({"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}));
    map.emplace('N', makeGlyph({"#   #", "##  #", "##  #", "# # #", "#  ##", "#  ##", "#   #"}));
    map.emplace('O', makeGlyph({" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}));
    map.emplace('P', makeGlyph({"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}));
    map.emplace('Q', makeGlyph({" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}));
    map.emplace('R', makeGlyph({"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}));
    map.emplace('S', makeGlyph({" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}));
    map.emplace('T', makeGlyph({"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}));
    map.emplace('U', makeGlyph({"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}));
    map.emplace('V', makeGlyph({"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}));
    map.emplace('W', makeGlyph({"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}));
    map.emplace('Y', makeGlyph({"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}));
    map.emplace('X', makeGlyph({"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}));
    map.emplace('Z', makeGlyph({"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}));

    map.emplace('=', makeGlyph({"     ", "#####", "     ", "#####", "     ", "     ", "     "}));
    map.emplace('-', makeGlyph({"     ", "     ", "#####", "     ", "     ", "     ", "     "}));
    map.emplace('+', makeGlyph({"  #  ", "  #  ", "#####", "  #  ", "  #  ", "     ", "     "}));
    map.emplace('.', makeGlyph({"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}));
    map.emplace(' ', makeGlyph({"     ", "     ", "     ", "     ", "     ", "     ", "     "}));

    return map;
  }();
  return kFont;
}

char sanitizeChar(char c) {
  if (std::isalpha(static_cast<unsigned char>(c))) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  if (std::isdigit(static_cast<unsigned char>(c))) {
    return c;
  }
  switch (c) {
    case '+':
    case '-':
    case '.':
    case '=':
    case ' ':
      return c;
    default:
      return ' ';
  }
}

std::string sanitizeText(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    result.push_back(sanitizeChar(c));
  }
  return result;
}

struct Color {
  std::array<std::uint8_t, 4> rgba{255, 255, 255, 255};
};

}  // namespace

namespace avs::effects {

struct ScriptedEffect::OverlayStyle {
  Color color;
};

ScriptedEffect::ScriptedEffect() = default;

void ScriptedEffect::setParams(const avs::core::ParamBlock& params) {
  rebuildScriptsFromParams(params);
  arbValParam_ = params.getFloat("arbval", arbValParam_);
}

void ScriptedEffect::rebuildScriptsFromParams(const avs::core::ParamBlock& params) {
  auto selectString = [&](const std::string& key, const std::string& fallback) {
    if (params.contains(key)) {
      return params.getString(key, fallback);
    }
    return fallback;
  };

  std::string newInit = selectString("init", initScript_);
  std::string newFrame = selectString("frame", frameScript_);
  std::string newPixel = selectString("pixel", pixelScript_);
  newFrame = selectString("code1", newFrame);
  newPixel = selectString("arbitrary", newPixel);
  newPixel = selectString("arbtxt", newPixel);
  std::string newLibrary = selectString("lib", libraryScript_);

  bool changed = false;
  if (newInit != initScript_) {
    initScript_ = std::move(newInit);
    changed = true;
  }
  if (newFrame != frameScript_) {
    frameScript_ = std::move(newFrame);
    changed = true;
  }
  if (newPixel != pixelScript_) {
    pixelScript_ = std::move(newPixel);
    changed = true;
  }
  if (newLibrary != libraryScript_) {
    libraryScript_ = std::move(newLibrary);
    changed = true;
  }

  if (changed) {
    dirty_ = true;
    initExecuted_ = false;
    compileErrorStage_.clear();
    compileErrorDetail_.clear();
    runtimeErrorStage_.clear();
    runtimeErrorDetail_.clear();
  }
}

void ScriptedEffect::ensureRuntime() {
  if (runtime_) {
    return;
  }
  runtime_ = std::make_unique<avs::runtime::script::EelRuntime>();
  time_ = runtime_->registerVar("time");
  frame_ = runtime_->registerVar("frame");
  widthVar_ = runtime_->registerVar("width");
  heightVar_ = runtime_->registerVar("height");
  xVar_ = runtime_->registerVar("x");
  yVar_ = runtime_->registerVar("y");
  redVar_ = runtime_->registerVar("red");
  greenVar_ = runtime_->registerVar("green");
  blueVar_ = runtime_->registerVar("blue");
  bassVar_ = runtime_->registerVar("bass");
  midVar_ = runtime_->registerVar("mid");
  trebVar_ = runtime_->registerVar("treb");
  arbValVar_ = runtime_->registerVar("arbval");
  for (std::size_t i = 0; i < globalVars_.size(); ++i) {
    const std::string name = "g" + std::to_string(i + 1);
    globalVars_[i] = runtime_->registerVar(name);
  }
}

bool ScriptedEffect::compileScripts() {
  compileErrorStage_.clear();
  compileErrorDetail_.clear();

  auto compose = [&](const std::string& body) {
    if (libraryScript_.empty()) {
      return body;
    }
    if (body.empty()) {
      return libraryScript_;
    }
    std::string combined = libraryScript_;
    combined.push_back('\n');
    combined += body;
    return combined;
  };

  std::string error;
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kInit, compose(initScript_), error)) {
    compileErrorStage_ = "INIT";
    compileErrorDetail_ = sanitizeText(error);
    return false;
  }
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kFrame, compose(frameScript_), error)) {
    compileErrorStage_ = "FRAME";
    compileErrorDetail_ = sanitizeText(error);
    return false;
  }
  if (!runtime_->compile(avs::runtime::script::EelRuntime::Stage::kPixel, compose(pixelScript_), error)) {
    compileErrorStage_ = "PIXEL";
    compileErrorDetail_ = sanitizeText(error);
    return false;
  }
  return true;
}

bool ScriptedEffect::render(avs::core::RenderContext& context) {
  ensureRuntime();

  if (dirty_) {
    if (!compileScripts()) {
      // leave initExecuted_ false to retry next time after params change.
    } else {
      initExecuted_ = false;
    }
    dirty_ = false;
  }

  runtimeErrorStage_.clear();
  runtimeErrorDetail_.clear();

  runtime_->setRandomSeed(context.rng.nextUint32());
  loadGlobalRegisters(context);
  timeSeconds_ += context.deltaSeconds;
  updateBindings(context);

  avs::runtime::script::ExecutionBudget budget;
  budget.maxInstructionBytes = kInstructionBudgetBytes;

  if (compileErrorStage_.empty()) {
    if (!initExecuted_) {
      if (executeStage(avs::runtime::script::EelRuntime::Stage::kInit, budget, "INIT")) {
        initExecuted_ = true;
      } else {
        initExecuted_ = true;
      }
    }
    if (runtimeErrorStage_.empty()) {
      executeStage(avs::runtime::script::EelRuntime::Stage::kFrame, budget, "FRAME");
    }
    if (runtimeErrorStage_.empty()) {
      applyPixelScript(context, budget);
    }
  }

  storeGlobalRegisters(context);
  drawOverlays(context);
  return runtimeErrorStage_.empty() && compileErrorStage_.empty();
}

bool ScriptedEffect::executeStage(avs::runtime::script::EelRuntime::Stage stage,
                                  avs::runtime::script::ExecutionBudget& budget,
                                  std::string_view label) {
  auto result = runtime_->execute(stage, &budget);
  if (!result.success) {
    runtimeErrorStage_ = std::string(label);
    runtimeErrorDetail_ = sanitizeText(result.message);
    if (runtimeErrorDetail_.empty()) {
      runtimeErrorDetail_ = "ERROR";
    }
    return false;
  }
  return true;
}

void ScriptedEffect::updateBindings(const avs::core::RenderContext& context) {
  if (widthVar_) *widthVar_ = static_cast<EEL_F>(context.width);
  if (heightVar_) *heightVar_ = static_cast<EEL_F>(context.height);
  if (time_) *time_ = static_cast<EEL_F>(timeSeconds_);
  if (frame_) *frame_ = static_cast<EEL_F>(context.frameIndex);
  if (arbValVar_) *arbValVar_ = static_cast<EEL_F>(arbValParam_);

  float bass = 0.0f;
  float mid = 0.0f;
  float treb = 0.0f;
  const auto& spectrum = context.audioSpectrum;
  if (spectrum.data && spectrum.size > 0) {
    const std::size_t third = std::max<std::size_t>(1, spectrum.size / 3);
    auto accumulateRange = [&](std::size_t begin, std::size_t end) {
      end = std::min(end, spectrum.size);
      double sum = 0.0;
      std::size_t count = 0;
      for (std::size_t i = begin; i < end; ++i) {
        sum += spectrum.data[i];
        ++count;
      }
      return count > 0 ? static_cast<float>(sum / static_cast<double>(count)) : 0.0f;
    };
    bass = accumulateRange(0, third);
    mid = accumulateRange(third, third * 2);
    treb = accumulateRange(third * 2, spectrum.size);
  }

  if (bassVar_) *bassVar_ = static_cast<EEL_F>(bass);
  if (midVar_) *midVar_ = static_cast<EEL_F>(mid);
  if (trebVar_) *trebVar_ = static_cast<EEL_F>(treb);
}

void ScriptedEffect::applyPixelScript(avs::core::RenderContext& context,
                                      avs::runtime::script::ExecutionBudget& budget) {
  if (context.width <= 0 || context.height <= 0) {
    return;
  }
  if (!context.framebuffer.data || context.framebuffer.size <
                                       static_cast<std::size_t>(context.width) *
                                           static_cast<std::size_t>(context.height) * 4u) {
    return;
  }

  auto clamp01 = [](double v) { return std::clamp(v, 0.0, 1.0); };

  for (int y = 0; y < context.height; ++y) {
    for (int x = 0; x < context.width; ++x) {
      const std::size_t idx =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(context.width) +
           static_cast<std::size_t>(x)) *
          4u;
      const double inR = context.framebuffer.data[idx] / 255.0;
      const double inG = context.framebuffer.data[idx + 1u] / 255.0;
      const double inB = context.framebuffer.data[idx + 2u] / 255.0;

      if (redVar_) *redVar_ = static_cast<EEL_F>(inR);
      if (greenVar_) *greenVar_ = static_cast<EEL_F>(inG);
      if (blueVar_) *blueVar_ = static_cast<EEL_F>(inB);

      double normX = 0.0;
      double normY = 0.0;
      if (context.width > 0) {
        normX = ((static_cast<double>(x) + 0.5) / static_cast<double>(context.width)) * 2.0 - 1.0;
      }
      if (context.height > 0) {
        normY = ((static_cast<double>(y) + 0.5) / static_cast<double>(context.height)) * 2.0 - 1.0;
      }
      if (xVar_) *xVar_ = static_cast<EEL_F>(normX);
      if (yVar_) *yVar_ = static_cast<EEL_F>(normY);

      auto result = runtime_->execute(avs::runtime::script::EelRuntime::Stage::kPixel, &budget);
      if (!result.success) {
        runtimeErrorStage_ = "PIXEL";
        runtimeErrorDetail_ = sanitizeText(result.message);
        if (runtimeErrorDetail_.empty()) {
          runtimeErrorDetail_ = "ERROR";
        }
        return;
      }

      const double outR = clamp01(redVar_ ? static_cast<double>(*redVar_) : inR);
      const double outG = clamp01(greenVar_ ? static_cast<double>(*greenVar_) : inG);
      const double outB = clamp01(blueVar_ ? static_cast<double>(*blueVar_) : inB);

      context.framebuffer.data[idx] = static_cast<std::uint8_t>(std::clamp(std::lround(outR * 255.0), 0l, 255l));
      context.framebuffer.data[idx + 1u] =
          static_cast<std::uint8_t>(std::clamp(std::lround(outG * 255.0), 0l, 255l));
      context.framebuffer.data[idx + 2u] =
          static_cast<std::uint8_t>(std::clamp(std::lround(outB * 255.0), 0l, 255l));
      context.framebuffer.data[idx + 3u] = 255u;
    }
  }
}

void ScriptedEffect::drawText(avs::core::RenderContext& context,
                              int originX,
                              int originY,
                              std::string_view text,
                              const OverlayStyle& style) const {
  if (!context.framebuffer.data) {
    return;
  }
  const auto& font = fontMap();
  std::string sanitized = sanitizeText(text);
  int cursorX = originX;
  for (char c : sanitized) {
    auto glyphIt = font.find(c);
    if (glyphIt == font.end()) {
      cursorX += kGlyphSpacing + 2;
      continue;
    }
    const Glyph& glyph = glyphIt->second;
    for (int col = 0; col < glyph.width; ++col) {
      std::uint8_t columnBits = glyph.columns[static_cast<std::size_t>(col)];
      for (int row = 0; row < kFontHeight; ++row) {
        if ((columnBits & (1u << row)) == 0u) {
          continue;
        }
        const int px = cursorX + col;
        const int py = originY + row;
        if (px < 0 || py < 0 || px >= context.width || py >= context.height) {
          continue;
        }
        const std::size_t idx = (static_cast<std::size_t>(py) * static_cast<std::size_t>(context.width) +
                                 static_cast<std::size_t>(px)) *
                                4u;
        context.framebuffer.data[idx] = style.color.rgba[0];
        context.framebuffer.data[idx + 1u] = style.color.rgba[1];
        context.framebuffer.data[idx + 2u] = style.color.rgba[2];
        context.framebuffer.data[idx + 3u] = 255u;
      }
    }
    cursorX += glyph.width + kGlyphSpacing;
  }
}

void ScriptedEffect::loadGlobalRegisters(const avs::core::RenderContext& context) {
  if (!runtime_) {
    return;
  }
  if (context.globals) {
    const auto& source = context.globals->registers;
    for (std::size_t i = 0; i < globalVars_.size(); ++i) {
      if (globalVars_[i]) {
        *globalVars_[i] = static_cast<EEL_F>(source[i]);
      }
    }
  } else {
    for (auto* var : globalVars_) {
      if (var) {
        *var = 0.0;
      }
    }
  }
}

void ScriptedEffect::storeGlobalRegisters(avs::core::RenderContext& context) const {
  if (!context.globals) {
    return;
  }
  auto& dest = context.globals->registers;
  for (std::size_t i = 0; i < globalVars_.size(); ++i) {
    if (globalVars_[i]) {
      dest[i] = static_cast<double>(*globalVars_[i]);
    }
  }
}

void ScriptedEffect::drawErrorOverlay(avs::core::RenderContext& context,
                                      int originY,
                                      std::string_view message) const {
  OverlayStyle style;
  style.color.rgba = {255, 64, 64, 255};
  drawText(context, 2, originY, message, style);
}

void ScriptedEffect::drawRegisterOverlay(avs::core::RenderContext& context, int originY) const {
  if (!runtime_) {
    return;
  }
  const auto values = runtime_->snapshotQ();
  OverlayStyle style;
  style.color.rgba = {255, 255, 255, 255};
  constexpr int rows = 8;
  constexpr int rowHeight = kFontHeight + 1;
  constexpr int colWidth = 72;
  for (int i = 0; i < 32; ++i) {
    const int row = i % rows;
    const int col = i / rows;
    const int x = 2 + col * colWidth;
    const int y = originY + row * rowHeight;
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Q%02d=%+.3f", i + 1, values[static_cast<std::size_t>(i)]);
    drawText(context, x, y, buffer, style);
  }
}

void ScriptedEffect::drawOverlays(avs::core::RenderContext& context) const {
  int offsetY = 2;
  if (!compileErrorStage_.empty()) {
    std::string message = "COMPILE " + compileErrorStage_;
    if (!compileErrorDetail_.empty()) {
      message += " " + compileErrorDetail_;
    }
    drawErrorOverlay(context, offsetY, message);
    offsetY += kFontHeight + 4;
  }
  if (!runtimeErrorStage_.empty()) {
    std::string message = runtimeErrorStage_;
    if (!runtimeErrorDetail_.empty()) {
      message += " " + runtimeErrorDetail_;
    }
    drawErrorOverlay(context, offsetY, message);
    offsetY += kFontHeight + 4;
  }
  drawRegisterOverlay(context, offsetY);
}

}  // namespace avs::effects

