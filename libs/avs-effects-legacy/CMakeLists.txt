add_library(avs-effects-legacy STATIC
  src/effect_registry.cpp
  src/trans/effect_transition.cpp
  src/trans/effect_fadeout.cpp
  src/misc/effect_unknown_render_object.cpp
  src/render/effect_oscilloscope_star.cpp
)

target_include_directories(avs-effects-legacy PUBLIC include)

target_compile_options(avs-effects-legacy PRIVATE -Wall -Wextra -Werror)
