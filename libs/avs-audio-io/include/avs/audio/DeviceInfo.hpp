#pragma once

#include <string>

namespace avs::audio {

struct DeviceInfo {
  int index = -1;
  std::string name;
  int maxInputChannels = 0;
  int maxOutputChannels = 0;
  double defaultSampleRate = 0.0;
  bool isDefaultInput = false;
  bool isDefaultOutput = false;

  bool isInputCapable() const;
  bool isOutputCapable() const;
  bool isFullDuplex() const;
};

}  // namespace avs::audio
