#include "avs/preset.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace avs {

namespace {
std::string trim(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

class PassThroughEffect : public Effect {
 public:
  void process(const Framebuffer& in, Framebuffer& out) override { out = in; }
};
}  // namespace

ParsedPreset parsePreset(const std::filesystem::path& file) {
  ParsedPreset result;
  std::ifstream f(file);
  if (!f) {
    result.warnings.push_back("failed to open preset");
    return result;
  }
  std::string line;
  while (std::getline(f, line)) {
    std::string t = trim(line);
    if (t.empty() || t[0] == '#') continue;
    std::istringstream iss(t);
    std::string type;
    iss >> type;
    if (type == "blur") {
      int radius = 5;
      std::string token;
      while (iss >> token) {
        if (token.rfind("radius=", 0) == 0) {
          radius = std::stoi(token.substr(7));
        } else {
          result.unknown.push_back(token);
        }
      }
      result.chain.push_back(std::make_unique<BlurEffect>(radius));
    } else if (type == "colormap") {
      result.chain.push_back(std::make_unique<ColorMapEffect>());
    } else if (type == "convolution") {
      result.chain.push_back(std::make_unique<ConvolutionEffect>());
    } else if (type == "scripted") {
      std::string script;
      std::getline(iss, script);
      script = trim(script);
      result.chain.push_back(std::make_unique<ScriptedEffect>(script, ""));
    } else {
      result.warnings.push_back("unsupported effect: " + type);
      result.chain.push_back(std::make_unique<PassThroughEffect>());
      result.unknown.push_back(t);
    }
  }
  return result;
}

}  // namespace avs
