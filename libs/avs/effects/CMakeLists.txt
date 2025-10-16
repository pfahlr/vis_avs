set(AVS_EFFECT_STUB_SOURCES
  ${CMAKE_SOURCE_DIR}/src/effects/effect.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_channel_shift.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_color_reduction.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_holden04_video_delay.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_holden05_multi_delay.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_misc_comment.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_misc_custom_bpm.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_misc_set_render_mode.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_multiplier.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_avi.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_bass_spin.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_dot_fountain.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_dot_plane.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_moving_particle.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_oscilloscope_star.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_ring.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_rotating_stars.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_simple.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_svp_loader.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_timescope.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_blitter_feedback.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_blur.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_brightness.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_color_clip.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_color_modifier.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_colorfade.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_mosaic.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_roto_blitter.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_scatter.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_unique_tone.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_water.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_water_bump.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/parser.cpp)

add_library(avs-effects-core
  src/Bump.cpp
  src/Blend.cpp
  src/Clear.cpp
  src/Globals.cpp
  src/Overlay.cpp
  src/Text.cpp
  src/RegisterEffects.cpp
  src/Swizzle.cpp
  src/Zoom.cpp
  src/effects/effect_scripted.cpp
  src/micro_preset_parser.cpp
  src/TransformAffine.cpp
  src/effects/blend_ops.cpp
  src/effects/gating.cpp
  src/effects/transform_affine.cpp
  src/AudioOverlays.cpp
  src/effects/primitive_dots.cpp
  src/effects/primitive_lines.cpp
  src/effects/primitive_rrect.cpp
  src/effects/primitive_solid.cpp
  src/effects/primitive_tri.cpp
  src/text/text_renderer.cpp
  ${AVS_EFFECT_STUB_SOURCES})


target_link_libraries(avs-effects-core PUBLIC avs-core-runtime avs-runtime)

target_include_directories(avs-effects-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_options(avs-effects-core PRIVATE -Wall -Wextra -Werror)

target_compile_features(avs-effects-core PUBLIC cxx_std_20)

install(TARGETS avs-effects-core
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
