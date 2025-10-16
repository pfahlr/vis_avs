#include "avs/effects/RegisterEffects.hpp"

#include <memory>

#include "avs/effects/AudioOverlays.hpp"
#include "avs/effects/Blend.hpp"
#include "avs/effects/Clear.hpp"
#include "avs/effects/Overlay.hpp"
#include "avs/effects/Swizzle.hpp"
#include "avs/effects/Zoom.hpp"

namespace avs::effects {

void registerCoreEffects(avs::core::EffectRegistry& registry) {
  registry.registerFactory("clear", []() { return std::make_unique<Clear>(); });
  registry.registerFactory("zoom", []() { return std::make_unique<Zoom>(); });
  registry.registerFactory("blend", []() { return std::make_unique<Blend>(); });
  registry.registerFactory("overlay", []() { return std::make_unique<Overlay>(); });
  registry.registerFactory("swizzle", []() { return std::make_unique<Swizzle>(); });
  registry.registerFactory("effect_wave",
                           []() { return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Wave); });
  registry.registerFactory("effect_spec", []() {
    return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Spectrum);
  });
  registry.registerFactory("effect_bands",
                           []() { return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Bands); });
  registry.registerFactory("effect_leveltext", []() {
    return std::make_unique<AudioOverlay>(AudioOverlay::Mode::LevelText);
  });
  registry.registerFactory("effect_bandtxt", []() {
    return std::make_unique<AudioOverlay>(AudioOverlay::Mode::BandText);
  });
}

}  // namespace avs::effects
