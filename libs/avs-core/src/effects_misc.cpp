#include "avs/effects_misc.hpp"
#include "avs/core.hpp"
#include "avs/params.hpp"
#include <algorithm>
#include <vector>

namespace avs {

// Effect List (stub)
void EffectListEffect::init(const InitContext&) {}
std::vector<Param> EffectListEffect::parameters() const { return {}; }
void EffectListEffect::process(const ProcessContext&, FrameBufferView&) {
  // In a real engine, this node would execute its child effects here.
}

// Global Variables (stub)
void GlobalVariablesEffect::init(const InitContext&) {}
std::vector<Param> GlobalVariablesEffect::parameters() const { return {}; }
void GlobalVariablesEffect::process(const ProcessContext&, FrameBufferView&) {}

// Save / Restore buffer (stubs)
std::vector<Param> SaveBufferEffect::parameters() const { return {}; }
void SaveBufferEffect::process(const ProcessContext&, FrameBufferView&) {}

std::vector<Param> RestoreBufferEffect::parameters() const { return {}; }
void RestoreBufferEffect::process(const ProcessContext&, FrameBufferView&) {}

// OnBeat Clear (stub)
std::vector<Param> OnBeatClearEffect::parameters() const { return { {"amount", ParamKind::Float, 1.0f, 0.0f, 1.0f} }; }
void OnBeatClearEffect::process(const ProcessContext& ctx, FrameBufferView& dst){
  if(!ctx.audio.beat) return;
  // Clear to black
  for(int y=0;y<dst.height;y++){
    uint8_t* row = dst.data + y * dst.stride;
    std::fill(row, row + dst.width*4, 0);
  }
}

// Clear Screen
std::vector<Param> ClearScreenEffect::parameters() const {
  return { {"color", ParamKind::Color, ColorRGBA8{0,0,0,255}} };
}

void ClearScreenEffect::process(const ProcessContext&, FrameBufferView& dst){
  for(int y=0;y<dst.height;y++){
    uint8_t* row = dst.data + y * dst.stride;
    for(int x=0;x<dst.width;x++){
      row[x*4+0]=0; row[x*4+1]=0; row[x*4+2]=0; row[x*4+3]=255;
    }
  }
}

} // namespace avs
