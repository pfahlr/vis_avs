#pragma once

#include <portaudio.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace avs::portaudio_detail {

struct CallbackResult {
  size_t nextWriteIndex;
  bool underflow;
};

CallbackResult processCallbackInput(const float* input, size_t samples, size_t writeIndex,
                                    size_t mask, std::vector<float>& ring);

bool callbackIndicatesUnderflow(const void* input, PaStreamCallbackFlags statusFlags);

struct StreamNegotiationRequest {
  int engineSampleRate = 0;
  int engineChannels = 0;
  std::optional<int> requestedSampleRate;
  std::optional<int> requestedChannels;
  bool useDeviceDefaultSampleRate = true;
  bool useDeviceDefaultChannels = false;
};

struct StreamNegotiationDeviceInfo {
  double defaultSampleRate = 0.0;
  int maxInputChannels = 0;
  int defaultInputChannels = 0;
};

struct StreamNegotiationResult {
  int channelCount = 0;
  double sampleRate = 0.0;
  bool usedFallbackRate = false;
  bool supported = false;
};

using FormatSupportQuery = std::function<bool(int, double)>;

StreamNegotiationResult negotiateStream(const StreamNegotiationRequest& request,
                                        const StreamNegotiationDeviceInfo& device,
                                        const FormatSupportQuery& isSupported);

struct DeviceSummary {
  int index = -1;
  std::string name;
  int maxInputChannels = 0;
};

struct DeviceSelectionResult {
  std::optional<int> index;
  std::string error;
};

DeviceSelectionResult resolveInputDeviceIdentifier(const std::optional<std::string>& identifier,
                                                   int deviceCount,
                                                   const std::vector<DeviceSummary>& devices);

}  // namespace avs::portaudio_detail
