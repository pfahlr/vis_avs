add_library(avs-compat)
add_library(avs::compat ALIAS avs-compat)
add_library(avs-runtime ALIAS avs-compat)

set(AVS_COMPAT_HEADERS
  include/avs/compat/core.hpp
  include/avs/compat/cpu_features.hpp
  include/avs/compat/effect.hpp
  include/avs/compat/effects.hpp
  include/avs/compat/effects_misc.hpp
  include/avs/compat/effects_render.hpp
  include/avs/compat/effects_trans.hpp
  include/avs/compat/eel.hpp
  include/avs/compat/engine.hpp
  include/avs/compat/framebuffers_bridge.h
  include/avs/compat/params.hpp
  include/avs/compat/preset.hpp
  include/avs/compat/registry.hpp
  include/avs/core.hpp
  include/avs/cpu_features.hpp
  include/avs/effect.hpp
  include/avs/effects.hpp
  include/avs/effects_misc.hpp
  include/avs/effects_render.hpp
  include/avs/effects_trans.hpp
  include/avs/eel.hpp
  include/avs/engine.hpp
  include/avs/framebuffers_bridge.h
  include/avs/params.hpp
  include/avs/preset.hpp
  include/avs/registry.hpp
  include/avs/runtime/GlobalState.hpp
  include/avs/runtime/ResourceManager.hpp
  include/avs/runtime/framebuffers.h
  include/runtime/framebuffers.h
)

set(AVS_COMPAT_SOURCES
  src/core.cpp
  src/cpu_features.cpp
  src/effects/additive_blend.cpp
  src/effects/blur.cpp
  src/effects/color_transform.cpp
  src/effects/colormap.cpp
  src/effects/convolution.cpp
  src/effects/composite.cpp
  src/effects/glow.cpp
  src/effects/mirror.cpp
  src/effects/motion_blur.cpp
  src/effects/radial_blur.cpp
  src/effects/scripted.cpp
  src/effects/tunnel.cpp
  src/effects/zoom_rotate.cpp
  src/effects/geometry/raster.cpp
  src/effects/geometry/superscope.cpp
  src/effects/geometry/text_renderer.cpp
  src/effects_misc.cpp
  src/effects_render.cpp
  src/effects_render_geometry.cpp
  src/effects_trans.cpp
  src/engine.cpp
  src/eel.cpp
  src/headless_main.cpp
  src/preset.cpp
  src/registry.cpp
  src/runtime/ResourceManager.cpp
  src/runtime/framebuffers.cpp
  src/runtime/framebuffers_bridge.cpp
)

target_sources(avs-compat
  PRIVATE
    ${AVS_COMPAT_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_COMPAT_HEADERS}
)

target_link_libraries(avs-compat
  PUBLIC
    avs::core
    avs::audio-io
  PRIVATE
    avs::audio-dsp
    ns-eel
)

target_compile_features(avs-compat PUBLIC cxx_std_20)
target_compile_options(avs-compat PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-compat
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS avs-compat
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
