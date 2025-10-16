#include <gtest/gtest.h>

#include <vector>

#include "avs/audio/AudioEngine.hpp"
#include "avs/audio/DeviceInfo.hpp"

namespace {

std::vector<avs::audio::DeviceInfo> makeDevices() {
  return {
      avs::audio::DeviceInfo{0, "InputOnly", 2, 0, 44100.0, false, false},
      avs::audio::DeviceInfo{1, "Duplex44100", 2, 2, 44100.0, false, false},
      avs::audio::DeviceInfo{2, "Duplex48000", 2, 2, 48000.0, true, true},
      avs::audio::DeviceInfo{3, "ExtraInput", 1, 0, 48000.0, false, false},
  };
}

}  // namespace

TEST(DeviceNegotiationTest, SelectsByExactNameOrIndex) {
  auto devices = makeDevices();
  auto byName = avs::audio::selectInputDevice(devices,
                                              avs::audio::DeviceSpecifier{std::string("Duplex48000")},
                                              48000.0);
  EXPECT_EQ("Duplex48000", byName.name);
  auto byIndex = avs::audio::selectInputDevice(devices, avs::audio::DeviceSpecifier{1}, 48000.0);
  EXPECT_EQ(1, byIndex.index);
}

TEST(DeviceNegotiationTest, ChoosesFirstDuplexWithSampleRateOtherwiseFirstInput) {
  auto devices = makeDevices();
  auto selected = avs::audio::selectInputDevice(devices, std::nullopt, 48000.0);
  EXPECT_EQ("Duplex48000", selected.name);

  std::vector<avs::audio::DeviceInfo> fallbackDevices{
      avs::audio::DeviceInfo{0, "InputOnly", 2, 0, 44100.0, true, false},
      avs::audio::DeviceInfo{1, "InputSecondary", 1, 0, 48000.0, false, false},
      avs::audio::DeviceInfo{2, "OutputOnly", 0, 2, 48000.0, false, false},
  };
  auto fallbackSelected = avs::audio::selectInputDevice(fallbackDevices, std::nullopt, 48000.0);
  EXPECT_EQ("InputOnly", fallbackSelected.name);
}

TEST(DeviceNegotiationTest, ThrowsWhenDeviceNotFound) {
  auto devices = makeDevices();
  EXPECT_THROW(avs::audio::selectInputDevice(devices, avs::audio::DeviceSpecifier{5}, 48000.0),
               std::runtime_error);
  EXPECT_THROW(avs::audio::selectInputDevice(devices, avs::audio::DeviceSpecifier{std::string("Missing")},
                                             48000.0),
               std::runtime_error);
}
