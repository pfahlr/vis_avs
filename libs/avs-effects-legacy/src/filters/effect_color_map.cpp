#include <avs/effects/legacy/filters/effect_color_map.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

#include <avs/effects/legacy/filters/filter_common.h>

namespace avs::effects::filters {

namespace {
using Channel = ColorMap::Channel;

Channel parseChannel(std::string_view value, Channel fallback) {
  if (value == "red" || value == "r") {
    return Channel::Red;
  }
  if (value == "green" || value == "g") {
    return Channel::Green;
  }
  if (value == "blue" || value == "b") {
    return Channel::Blue;
  }
  if (value == "alpha" || value == "a") {
    return Channel::Alpha;
  }
  if (value == "luma" || value == "y" || value == "brightness") {
    return Channel::Luma;
  }
  return fallback;
}

std::string sanitizeToken(std::string token) {
  token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char ch) {
                return ch == ',' || ch == ';' || std::isspace(ch) != 0;
              }),
              token.end());
  if (!token.empty() && token.front() == '#') {
    token.erase(token.begin());
  }
  if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
    token.erase(token.begin(), token.begin() + 2);
  }
  return token;
}

std::uint32_t parseHex(std::string_view token) {
  std::uint32_t value = 0;
  std::stringstream ss;
  ss << std::hex << token;
  ss >> value;
  return value;
}

}  // namespace

ColorMap::ColorMap() {
  for (std::size_t i = 0; i < table_.size(); ++i) {
    table_[i] = {static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), 255u};
  }
}

void ColorMap::parseTable(std::string_view tableText) {
  for (std::size_t i = 0; i < table_.size(); ++i) {
    table_[i] = {static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), static_cast<std::uint8_t>(i), 255u};
  }
  if (tableText.empty()) {
    return;
  }

  std::stringstream stream{std::string(tableText)};
  std::string token;
  std::size_t index = 0;
  auto applyToken = [&](std::string_view rawToken) {
    if (index >= table_.size()) {
      return;
    }
    std::string tokenValue = sanitizeToken(std::string(rawToken));
    if (tokenValue.empty()) {
      return;
    }
    const std::uint32_t value = parseHex(tokenValue);
    std::array<std::uint8_t, 4> entry{};
    if (tokenValue.size() <= 6) {
      entry[0] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
      entry[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
      entry[2] = static_cast<std::uint8_t>(value & 0xFFu);
      entry[3] = 255u;
    } else {
      entry[3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
      entry[0] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
      entry[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
      entry[2] = static_cast<std::uint8_t>(value & 0xFFu);
    }
    table_[index++] = entry;
  };

  while (stream >> token && index < table_.size()) {
    if (token.empty()) {
      continue;
    }
    std::size_t start = 0;
    while (start < token.size() && index < table_.size()) {
      const std::size_t end = token.find_first_of(",;", start);
      const std::size_t length = (end == std::string::npos) ? token.size() - start : end - start;
      applyToken(std::string_view(token).substr(start, length));
      if (end == std::string::npos) {
        break;
      }
      start = end + 1;
    }
  }

  if (index > 0 && index < table_.size()) {
    const std::array<std::uint8_t, 4> last = table_[index - 1];
    for (; index < table_.size(); ++index) {
      table_[index] = last;
    }
  }
}

void ColorMap::setParams(const avs::core::ParamBlock& params) {
  std::string tableString = params.getString("table", params.getString("map", ""));
  if (!tableString.empty()) {
    parseTable(tableString);
  }
  std::string channelString = params.getString("channel", "");
  std::transform(channelString.begin(), channelString.end(), channelString.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  if (!channelString.empty()) {
    channel_ = parseChannel(channelString, channel_);
  }
  mapAlpha_ = params.getBool("map_alpha", mapAlpha_);
  invert_ = params.getBool("invert", invert_);
}

std::uint8_t ColorMap::indexFromPixel(const std::uint8_t* pixel) const {
  int index = 0;
  switch (channel_) {
    case Channel::Red:
      index = pixel[0];
      break;
    case Channel::Green:
      index = pixel[1];
      break;
    case Channel::Blue:
      index = pixel[2];
      break;
    case Channel::Alpha:
      index = pixel[3];
      break;
    case Channel::Luma:
    default:
      index = (pixel[0] * 54 + pixel[1] * 183 + pixel[2] * 19) >> 8;
      break;
  }
  if (invert_) {
    index = 255 - index;
  }
  return static_cast<std::uint8_t>(std::clamp(index, 0, 255));
}

bool ColorMap::render(avs::core::RenderContext& context) {
  if (!hasFramebuffer(context)) {
    return true;
  }

  std::uint8_t* pixels = context.framebuffer.data;
  const std::size_t totalPixels = static_cast<std::size_t>(context.width) * static_cast<std::size_t>(context.height);
  for (std::size_t i = 0; i < totalPixels; ++i) {
    std::uint8_t* px = pixels + i * 4u;
    const std::array<std::uint8_t, 4>& mapped = table_[indexFromPixel(px)];
    px[0] = mapped[0];
    px[1] = mapped[1];
    px[2] = mapped[2];
    if (mapAlpha_) {
      px[3] = mapped[3];
    }
  }
  return true;
}

}  // namespace avs::effects::filters

