#include <avs/preset/json.h>
#include <avs/preset/parser.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void printUsage(const char* progName) {
  std::fprintf(stderr,
               "Usage: %s --input <preset-file> --output <json-file>\n\n"
               "Convert AVS preset files to human-readable JSON format.\n\n"
               "Options:\n"
               "  --input  <file>   Input preset file (.avs or text format)\n"
               "  --output <file>   Output JSON file (.json)\n"
               "  --help            Show this help message\n\n"
               "Example:\n"
               "  %s --input mypreset.avs --output mypreset.json\n",
               progName, progName);
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to create file: " + path.string());
  }
  file << content;
  if (!file) {
    throw std::runtime_error("Failed to write to file: " + path.string());
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  std::filesystem::path inputPath;
  std::filesystem::path outputPath;

  // Parse command-line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage(argv[0]);
      return 0;
    } else if (arg == "--input" && i + 1 < argc) {
      inputPath = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      outputPath = argv[++i];
    } else {
      std::fprintf(stderr, "Unknown argument: %s\n\n", arg.c_str());
      printUsage(argv[0]);
      return 1;
    }
  }

  // Validate arguments
  if (inputPath.empty() || outputPath.empty()) {
    std::fprintf(stderr, "Error: Both --input and --output are required\n\n");
    printUsage(argv[0]);
    return 1;
  }

  if (!std::filesystem::exists(inputPath)) {
    std::fprintf(stderr, "Error: Input file does not exist: %s\n", inputPath.string().c_str());
    return 1;
  }

  try {
    // Read input file
    std::printf("Reading preset: %s\n", inputPath.string().c_str());
    std::string presetData = readFile(inputPath);

    // Parse the preset using legacy parser
    std::printf("Parsing preset...\n");
    avs::preset::IRPreset preset = avs::preset::parse_legacy_preset(presetData);

    // Check if any effects were parsed
    if (preset.root_nodes.empty()) {
      std::fprintf(stderr, "Warning: Preset appears to be empty or could not be parsed\n");
    } else {
      std::printf("Parsed %zu effect(s)\n", preset.root_nodes.size());
    }

    // Convert to JSON
    std::printf("Converting to JSON...\n");
    std::string json = avs::preset::serializeToJson(preset, 2);

    // Write output file
    std::printf("Writing JSON: %s\n", outputPath.string().c_str());
    writeFile(outputPath, json);

    std::printf("✓ Conversion successful!\n");
    std::printf("  Input:  %s\n", inputPath.string().c_str());
    std::printf("  Output: %s (%zu bytes)\n", outputPath.string().c_str(), json.size());

    return 0;

  } catch (const std::exception& e) {
    std::fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }
}
