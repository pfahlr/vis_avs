add_library(avs-core STATIC
  src/core.cpp
  src/engine.cpp
  src/eel.cpp
  src/cpu_features.cpp
  src/registry.cpp
  src/effects_render.cpp
  src/effects_render_geometry.cpp
  src/effects_trans.cpp
  src/effects_misc.cpp
  src/effects/blur.cpp
  src/effects/colormap.cpp
  src/effects/convolution.cpp
  src/effects/motion_blur.cpp
  src/effects/fadeout.cpp
  src/effects/color_transform.cpp
  src/effects/glow.cpp
  src/effects/zoom_rotate.cpp
  src/effects/mirror.cpp
  src/effects/tunnel.cpp
  src/effects/radial_blur.cpp
  src/effects/additive_blend.cpp
  src/effects/composite.cpp
  src/effects/scripted.cpp
  src/effects/geometry/raster.cpp
  src/effects/geometry/text_renderer.cpp
  src/effects/geometry/superscope.cpp
  src/preset.cpp
)
target_include_directories(avs-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/avs-platform/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/avs/runtime/include>
    ${CMAKE_SOURCE_DIR}/libs/third_party
)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-core PUBLIC cxx_std_20)
# Required for Framebuffers::save/restore used by SaveBufferEffect and RestoreBufferEffect
target_link_libraries(avs-core
  PUBLIC
    ns-eel
    avs-runtime
)
add_executable(avs_headless src/headless_main.cpp)
target_link_libraries(avs_headless PRIVATE avs-core)
target_compile_options(avs_headless PRIVATE -Wall -Wextra -Werror)

install(TARGETS avs-core avs_headless
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
