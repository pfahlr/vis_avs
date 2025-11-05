add_library(avs-compat STATIC
  src/effects_common.cpp
  src/effects_render.cpp
  src/effects_trans.cpp
  src/effects_misc.cpp
)

target_include_directories(avs-compat PUBLIC include)

target_link_libraries(avs-compat PUBLIC avs-effects-legacy)

target_compile_options(avs-compat PRIVATE -Wall -Wextra -Werror)
