#include "effects/stubs/effect_channel_shift.h"


Effect_ChannelShift::Effect_ChannelShift() : Effect("Channel Shift") {}

bool Effect_ChannelShift::render(avs::core::RenderContext& context) {
  (void)context;
  // TODO: Implement per R_ChannelShift in rlib.cpp
  return true;
}
