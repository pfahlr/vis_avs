#include "avs/effects/RegisterEffects.hpp"

#include <memory>

#include "avs/effects/AudioOverlays.hpp"
#include "avs/effects/Blend.hpp"
#include "avs/effects/Clear.hpp"
#include "avs/effects/Overlay.hpp"
#include "avs/effects/Primitives.hpp"
#include "avs/effects/Swizzle.hpp"
#include "avs/effects/TransformAffine.hpp"
#include "avs/effects/Zoom.hpp"

namespace avs::effects {

void registerCoreEffects(avs::core::EffectRegistry& registry) {
  registry.registerFactory("clear", []() { return std::make_unique<Clear>(); });
  registry.registerFactory("zoom", []() { return std::make_unique<Zoom>(); });
  registry.registerFactory("blend", []() { return std::make_unique<Blend>(); });
  registry.registerFactory("overlay", []() { return std::make_unique<Overlay>(); });
  registry.registerFactory("swizzle", []() { return std::make_unique<Swizzle>(); });
  registry.registerFactory("transform_affine", []() { return std::make_unique<TransformAffine>(); });
  registry.registerFactory("effect_wave", []() { return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Wave); });
  registry.registerFactory("effect_spec", []() { return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Spectrum);});
  registry.registerFactory("effect_bands", []() { return std::make_unique<AudioOverlay>(AudioOverlay::Mode::Bands); });
  registry.registerFactory("effect_leveltext", []() {return std::make_unique<AudioOverlay>(AudioOverlay::Mode::LevelText);});
  registry.registerFactory("effect_bandtxt", []() {return std::make_unique<AudioOverlay>(AudioOverlay::Mode::BandText);});
  registry.registerFactory("solid", []() { return std::make_unique<PrimitiveSolid>(); });
  registry.registerFactory("dot", []() { return std::make_unique<PrimitiveDots>(); });
  registry.registerFactory("dots", []() { return std::make_unique<PrimitiveDots>(); });
  registry.registerFactory("line", []() { return std::make_unique<PrimitiveLines>(); });
  registry.registerFactory("lines", []() { return std::make_unique<PrimitiveLines>(); });
  registry.registerFactory("tri", []() { return std::make_unique<PrimitiveTriangles>(); });
  registry.registerFactory("triangle", []() { return std::make_unique<PrimitiveTriangles>(); });
  registry.registerFactory("triangles", []() { return std::make_unique<PrimitiveTriangles>(); });
  registry.registerFactory("rrect", []() { return std::make_unique<PrimitiveRoundedRect>(); });
  registry.registerFactory("roundedrect", []() { return std::make_unique<PrimitiveRoundedRect>(); });
  registry.registerFactory("text", []() { return std::make_unique<Text>(); });
}

}  // namespace avs::effects
