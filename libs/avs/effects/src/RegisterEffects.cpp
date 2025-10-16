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
#include "effects/filters/effect_blur_box.h"
#include "effects/filters/effect_color_map.h"
#include "effects/filters/effect_conv3x3.h"
#include "effects/filters/effect_fast_brightness.h"
#include "effects/filters/effect_grain.h"
#include "effects/filters/effect_interferences.h"
#include "effects/effect_scripted.h"
#include "effects/stubs/effect_channel_shift.h"
#include "effects/stubs/effect_color_reduction.h"
#include "effects/stubs/effect_holden04_video_delay.h"
#include "effects/stubs/effect_holden05_multi_delay.h"
#include "effects/stubs/effect_misc_comment.h"
#include "effects/stubs/effect_misc_custom_bpm.h"
#include "effects/stubs/effect_misc_set_render_mode.h"
#include "effects/stubs/effect_multiplier.h"
#include "effects/stubs/effect_render_avi.h"
#include "effects/stubs/effect_render_bass_spin.h"
#include "effects/stubs/effect_render_dot_fountain.h"
#include "effects/stubs/effect_render_dot_plane.h"
#include "effects/stubs/effect_render_moving_particle.h"
#include "effects/stubs/effect_render_oscilloscope_star.h"
#include "effects/stubs/effect_render_ring.h"
#include "effects/stubs/effect_render_rotating_stars.h"
#include "effects/stubs/effect_render_simple.h"
#include "effects/stubs/effect_render_svp_loader.h"
#include "effects/stubs/effect_render_timescope.h"
#include "effects/stubs/effect_trans_blitter_feedback.h"
#include "effects/stubs/effect_trans_blur.h"
#include "effects/stubs/effect_trans_brightness.h"
#include "effects/stubs/effect_trans_color_clip.h"
#include "effects/stubs/effect_trans_color_modifier.h"
#include "effects/stubs/effect_trans_colorfade.h"
#include "effects/stubs/effect_trans_mosaic.h"
#include "effects/stubs/effect_trans_roto_blitter.h"
#include "effects/stubs/effect_trans_scatter.h"
#include "effects/stubs/effect_trans_unique_tone.h"
#include "effects/stubs/effect_trans_water.h"
#include "effects/stubs/effect_trans_water_bump.h"

namespace avs::effects {

void registerCoreEffects(avs::core::EffectRegistry& registry) {
  registry.registerFactory("clear", []() { return std::make_unique<Clear>(); });
  registry.registerFactory("zoom", []() { return std::make_unique<Zoom>(); });
  registry.registerFactory("blend", []() { return std::make_unique<Blend>(); });
  registry.registerFactory("overlay", []() { return std::make_unique<Overlay>(); });
  registry.registerFactory("swizzle", []() { return std::make_unique<Swizzle>(); });
  registry.registerFactory("scripted", []() { return std::make_unique<ScriptedEffect>(); });
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

  registry.registerFactory("Channel Shift", []() { return std::make_unique<Effect_ChannelShift>(); });
  registry.registerFactory("channel shift", []() { return std::make_unique<Effect_ChannelShift>(); });
  registry.registerFactory("Color Reduction", []() { return std::make_unique<Effect_ColorReduction>(); });
  registry.registerFactory("color reduction", []() { return std::make_unique<Effect_ColorReduction>(); });
  registry.registerFactory("Holden04: Video Delay", []() { return std::make_unique<Effect_Holden04VideoDelay>(); });
  registry.registerFactory("holden04: video delay", []() { return std::make_unique<Effect_Holden04VideoDelay>(); });
  registry.registerFactory("Holden05: Multi Delay", []() { return std::make_unique<Effect_Holden05MultiDelay>(); });
  registry.registerFactory("holden05: multi delay", []() { return std::make_unique<Effect_Holden05MultiDelay>(); });
  registry.registerFactory("Misc / Comment", []() { return std::make_unique<Effect_MiscComment>(); });
  registry.registerFactory("misc / comment", []() { return std::make_unique<Effect_MiscComment>(); });
  registry.registerFactory("Misc / Custom BPM", []() { return std::make_unique<Effect_MiscCustomBpm>(); });
  registry.registerFactory("misc / custom bpm", []() { return std::make_unique<Effect_MiscCustomBpm>(); });
  registry.registerFactory("Misc / Set render mode", []() { return std::make_unique<Effect_MiscSetRenderMode>(); });
  registry.registerFactory("misc / set render mode", []() { return std::make_unique<Effect_MiscSetRenderMode>(); });
  registry.registerFactory("Multiplier", []() { return std::make_unique<Effect_Multiplier>(); });
  registry.registerFactory("multiplier", []() { return std::make_unique<Effect_Multiplier>(); });
  registry.registerFactory("Render / AVI", []() { return std::make_unique<Effect_RenderAvi>(); });
  registry.registerFactory("render / avi", []() { return std::make_unique<Effect_RenderAvi>(); });
  registry.registerFactory("Render / Bass Spin", []() { return std::make_unique<Effect_RenderBassSpin>(); });
  registry.registerFactory("render / bass spin", []() { return std::make_unique<Effect_RenderBassSpin>(); });
  registry.registerFactory("Render / Dot Fountain", []() { return std::make_unique<Effect_RenderDotFountain>(); });
  registry.registerFactory("render / dot fountain", []() { return std::make_unique<Effect_RenderDotFountain>(); });
  registry.registerFactory("Render / Dot Plane", []() { return std::make_unique<Effect_RenderDotPlane>(); });
  registry.registerFactory("render / dot plane", []() { return std::make_unique<Effect_RenderDotPlane>(); });
  registry.registerFactory("Render / Moving Particle", []() { return std::make_unique<Effect_RenderMovingParticle>(); });
  registry.registerFactory("render / moving particle", []() { return std::make_unique<Effect_RenderMovingParticle>(); });
  registry.registerFactory("Render / Oscilloscope Star", []() { return std::make_unique<Effect_RenderOscilloscopeStar>(); });
  registry.registerFactory("render / oscilloscope star", []() { return std::make_unique<Effect_RenderOscilloscopeStar>(); });
  registry.registerFactory("Render / Ring", []() { return std::make_unique<Effect_RenderRing>(); });
  registry.registerFactory("render / ring", []() { return std::make_unique<Effect_RenderRing>(); });
  registry.registerFactory("Render / Rotating Stars", []() { return std::make_unique<Effect_RenderRotatingStars>(); });
  registry.registerFactory("render / rotating stars", []() { return std::make_unique<Effect_RenderRotatingStars>(); });
  registry.registerFactory("Render / Simple", []() { return std::make_unique<Effect_RenderSimple>(); });
  registry.registerFactory("render / simple", []() { return std::make_unique<Effect_RenderSimple>(); });
  registry.registerFactory("Render / SVP Loader", []() { return std::make_unique<Effect_RenderSvpLoader>(); });
  registry.registerFactory("render / svp loader", []() { return std::make_unique<Effect_RenderSvpLoader>(); });
  registry.registerFactory("Render / Timescope", []() { return std::make_unique<Effect_RenderTimescope>(); });
  registry.registerFactory("render / timescope", []() { return std::make_unique<Effect_RenderTimescope>(); });
  registry.registerFactory("Trans / Blitter Feedback", []() { return std::make_unique<Effect_TransBlitterFeedback>(); });
  registry.registerFactory("trans / blitter feedback", []() { return std::make_unique<Effect_TransBlitterFeedback>(); });
  registry.registerFactory("filter_blur_box", []() { return std::make_unique<filters::BlurBox>(); });
  registry.registerFactory("Trans / Blur", []() { return std::make_unique<filters::BlurBox>(); });
  registry.registerFactory("trans / blur", []() { return std::make_unique<filters::BlurBox>(); });
  registry.registerFactory("filter_grain", []() { return std::make_unique<filters::Grain>(); });
  registry.registerFactory("Trans / Grain", []() { return std::make_unique<filters::Grain>(); });
  registry.registerFactory("trans / grain", []() { return std::make_unique<filters::Grain>(); });
  registry.registerFactory("filter_interferences", []() { return std::make_unique<filters::Interferences>(); });
  registry.registerFactory("Trans / Interferences", []() { return std::make_unique<filters::Interferences>(); });
  registry.registerFactory("trans / interferences", []() { return std::make_unique<filters::Interferences>(); });
  registry.registerFactory("filter_fast_brightness", []() { return std::make_unique<filters::FastBrightness>(); });
  registry.registerFactory("Trans / Fast Brightness", []() { return std::make_unique<filters::FastBrightness>(); });
  registry.registerFactory("trans / fast brightness", []() { return std::make_unique<filters::FastBrightness>(); });
  registry.registerFactory("filter_color_map", []() { return std::make_unique<filters::ColorMap>(); });
  registry.registerFactory("Filter / Color Map", []() { return std::make_unique<filters::ColorMap>(); });
  registry.registerFactory("filter / color map", []() { return std::make_unique<filters::ColorMap>(); });
  registry.registerFactory("filter_conv3x3", []() { return std::make_unique<filters::Convolution3x3>(); });
  registry.registerFactory("Filter / Convolution", []() { return std::make_unique<filters::Convolution3x3>(); });
  registry.registerFactory("filter / convolution", []() { return std::make_unique<filters::Convolution3x3>(); });
  registry.registerFactory("Trans / Brightness", []() { return std::make_unique<Effect_TransBrightness>(); });
  registry.registerFactory("trans / brightness", []() { return std::make_unique<Effect_TransBrightness>(); });
  registry.registerFactory("Trans / Color Clip", []() { return std::make_unique<Effect_TransColorClip>(); });
  registry.registerFactory("trans / color clip", []() { return std::make_unique<Effect_TransColorClip>(); });
  registry.registerFactory("Trans / Color Modifier", []() { return std::make_unique<Effect_TransColorModifier>(); });
  registry.registerFactory("trans / color modifier", []() { return std::make_unique<Effect_TransColorModifier>(); });
  registry.registerFactory("Trans / Colorfade", []() { return std::make_unique<Effect_TransColorfade>(); });
  registry.registerFactory("trans / colorfade", []() { return std::make_unique<Effect_TransColorfade>(); });
  registry.registerFactory("Trans / Mosaic", []() { return std::make_unique<Effect_TransMosaic>(); });
  registry.registerFactory("trans / mosaic", []() { return std::make_unique<Effect_TransMosaic>(); });
  registry.registerFactory("Trans / Roto Blitter", []() { return std::make_unique<Effect_TransRotoBlitter>(); });
  registry.registerFactory("trans / roto blitter", []() { return std::make_unique<Effect_TransRotoBlitter>(); });
  registry.registerFactory("Trans / Scatter", []() { return std::make_unique<Effect_TransScatter>(); });
  registry.registerFactory("trans / scatter", []() { return std::make_unique<Effect_TransScatter>(); });
  registry.registerFactory("Trans / Unique tone", []() { return std::make_unique<Effect_TransUniqueTone>(); });
  registry.registerFactory("trans / unique tone", []() { return std::make_unique<Effect_TransUniqueTone>(); });
  registry.registerFactory("Trans / Water", []() { return std::make_unique<Effect_TransWater>(); });
  registry.registerFactory("trans / water", []() { return std::make_unique<Effect_TransWater>(); });
  registry.registerFactory("Trans / Water Bump", []() { return std::make_unique<Effect_TransWaterBump>(); });
  registry.registerFactory("trans / water bump", []() { return std::make_unique<Effect_TransWaterBump>(); });

}

}  // namespace avs::effects
