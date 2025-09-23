#pragma once

#include <portaudio.h>

#include <cstddef>
#include <functional>
#include <optional>
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
  int engineSampleRate;
  int engineChannels;
  std::optional<int> requestedSampleRate;
  std::optional<int> requestedChannels;
};

struct StreamNegotiationDeviceInfo {
  double defaultSampleRate;
  int maxInputChannels;
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

}  // namespace avs::portaudio_detail
