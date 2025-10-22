#include <avs/audio/DeviceInfo.hpp>

namespace avs::audio {

bool DeviceInfo::isInputCapable() const { return maxInputChannels > 0; }

bool DeviceInfo::isOutputCapable() const { return maxOutputChannels > 0; }

bool DeviceInfo::isFullDuplex() const { return isInputCapable() && isOutputCapable(); }

}  // namespace avs::audio
