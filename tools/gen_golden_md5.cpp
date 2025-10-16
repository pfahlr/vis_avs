#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "avs/offscreen/Md5.hpp"
#include "avs/offscreen/OffscreenRenderer.hpp"

namespace {

struct Options {
  int frames = 10;
  int width = 320;
  int height = 240;
  int seed = 1234;
  std::filesystem::path preset;
};

void printUsage() {
  std::cout << "Usage: gen_golden_md5 [--frames N] [--width W] [--height H] [--seed S] [--preset FILE]\n";
}

std::string requireValue(int& index, int argc, char** argv) {
  if (index + 1 >= argc) {
    throw std::runtime_error("missing value for argument: " + std::string(argv[index]));
  }
  ++index;
  return argv[index];
}

Options parseOptions(int argc, char** argv) {
  Options opts;
  opts.preset = std::filesystem::path(SOURCE_DIR) / "tests/regression/data/tiny_preset_fragment.avs";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage();
      std::exit(0);
    } else if (arg == "--frames") {
      opts.frames = std::stoi(requireValue(i, argc, argv));
    } else if (arg == "--width") {
      opts.width = std::stoi(requireValue(i, argc, argv));
    } else if (arg == "--height") {
      opts.height = std::stoi(requireValue(i, argc, argv));
    } else if (arg == "--seed") {
      opts.seed = std::stoi(requireValue(i, argc, argv));
    } else if (arg == "--preset") {
      opts.preset = requireValue(i, argc, argv);
    } else {
      throw std::runtime_error("unknown argument: " + arg);
    }
  }

  if (opts.frames <= 0) {
    throw std::runtime_error("--frames must be positive");
  }
  if (opts.width <= 0 || opts.height <= 0) {
    throw std::runtime_error("--width and --height must be positive");
  }
  if (opts.preset.empty()) {
    throw std::runtime_error("--preset must not be empty");
  }
  return opts;
}

std::vector<float> generateAudioBuffer(unsigned sampleRate, unsigned channels, double silenceSeconds,
                                       double toneSeconds, double frequencyHz) {
  const std::size_t silenceFrames = static_cast<std::size_t>(silenceSeconds * static_cast<double>(sampleRate));
  const std::size_t toneFrames = static_cast<std::size_t>(toneSeconds * static_cast<double>(sampleRate));
  const std::size_t totalFrames = silenceFrames + toneFrames;
  std::vector<float> samples(totalFrames * static_cast<std::size_t>(channels), 0.0f);
  const double twoPiF = 2.0 * 3.14159265358979323846 * frequencyHz;
  for (std::size_t frame = silenceFrames; frame < totalFrames; ++frame) {
    const double t = static_cast<double>(frame - silenceFrames) / static_cast<double>(sampleRate);
    const float value = static_cast<float>(std::sin(twoPiF * t));
    for (unsigned ch = 0; ch < channels; ++ch) {
      samples[frame * static_cast<std::size_t>(channels) + ch] = value;
    }
  }
  return samples;
}

void applySeed(int seed) {
  const std::string value = std::to_string(seed);
#ifdef _WIN32
  _putenv_s("AVS_SEED", value.c_str());
#else
  ::setenv("AVS_SEED", value.c_str(), 1);
#endif
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options opts = parseOptions(argc, argv);
    applySeed(opts.seed);

    avs::offscreen::OffscreenRenderer renderer(opts.width, opts.height);
    renderer.loadPreset(opts.preset);

    auto audio = generateAudioBuffer(48000, 2, 0.05, 0.5, 1000.0);
    renderer.setAudioBuffer(std::move(audio), 48000, 2);

    std::vector<std::string> md5Values;
    md5Values.reserve(static_cast<std::size_t>(opts.frames));

    for (int frame = 0; frame < opts.frames; ++frame) {
      auto view = renderer.render();
      if (view.data == nullptr || view.size == 0) {
        throw std::runtime_error("received empty frame data");
      }
      md5Values.push_back(avs::offscreen::computeMd5Hex(view.data, view.size));
    }

    std::cout << "{\n";
    std::cout << "  \"width\": " << opts.width << ",\n";
    std::cout << "  \"height\": " << opts.height << ",\n";
    std::cout << "  \"seed\": " << opts.seed << ",\n";
    std::cout << "  \"md5\": [\n";
    for (std::size_t i = 0; i < md5Values.size(); ++i) {
      std::cout << "    \"" << md5Values[i] << "\"";
      if (i + 1 < md5Values.size()) {
        std::cout << ",";
      }
      std::cout << "\n";
    }
    std::cout << "  ]\n";
    std::cout << "}\n";
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}

