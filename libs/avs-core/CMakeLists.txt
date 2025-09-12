add_library(avs-core STATIC
  src/core.cpp
  src/engine.cpp
  src/eel.cpp
  src/effects/blur.cpp
  src/effects/colormap.cpp
  src/effects/convolution.cpp
  src/effects/scripted.cpp
  src/preset.cpp
)
target_include_directories(avs-core PUBLIC include ../avs-platform/include)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)
target_link_libraries(avs-core PUBLIC ns-eel)
