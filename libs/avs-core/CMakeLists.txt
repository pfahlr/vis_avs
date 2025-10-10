cmake_minimum_required(VERSION 3.16)
project(avs_scaffold LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(AVS_CORE_SOURCES
  src/core.cpp
  src/core/pixel_ops.cpp
  src/engine.cpp
  src/cpu_features.cpp
  src/registry.cpp
  src/effects_render.cpp
  src/effects_trans.cpp
  src/effects_misc.cpp
  src/effects/blur.cpp
  src/effects/colormap.cpp
  src/effects/convolution.cpp
  src/effects/motion_blur.cpp
  src/effects/color_transform.cpp
  src/effects/glow.cpp
  src/effects/zoom_rotate.cpp
  src/effects/mirror.cpp
  src/effects/tunnel.cpp
  src/effects/radial_blur.cpp
  src/effects/additive_blend.cpp
  src/effects/composite.cpp
  src/preset.cpp
  src/script/EELContext.cpp
)

if(ENABLE_EEL2)
  list(APPEND AVS_CORE_SOURCES
    src/eel.cpp
    src/effects/scripted.cpp)
endif()

add_library(avs-core STATIC ${AVS_CORE_SOURCES})
target_include_directories(avs-core PUBLIC include ../avs-platform/include)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)
if(ENABLE_EEL2)
  target_link_libraries(avs-core PUBLIC ns-eel)
endif()

target_compile_definitions(avs-core PUBLIC AVS_ENABLE_EEL2=$<BOOL:${ENABLE_EEL2}>)
add_executable(avs_headless src/headless_main.cpp)
target_link_libraries(avs_headless PRIVATE avs-core)
