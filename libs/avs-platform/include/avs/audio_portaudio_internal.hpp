#pragma once

#include <cstddef>
#include <vector>

namespace avs::portaudio_detail {

struct CallbackResult {
  size_t nextWriteIndex;
  bool underflow;
};

CallbackResult processCallbackInput(const float* input, size_t samples, size_t writeIndex,
                                    size_t mask, std::vector<float>& ring);

}  // namespace avs::portaudio_detail
