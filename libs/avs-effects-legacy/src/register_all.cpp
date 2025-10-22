#include <avs/effects/legacy/register_all.h>

namespace avs::effects::legacy {

void register_color_modifier(Registry&);

void register_all(Registry& r) { register_color_modifier(r); }

}  // namespace avs::effects::legacy
