set(AVS_EFFECT_STUB_SOURCES
  ${CMAKE_SOURCE_DIR}/src/effects/effect.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_bass_spin.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_moving_particle.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_oscilloscope_star.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_avi.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_svp_loader.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_render_timescope.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_brightness.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_color_clip.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_colorfade.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_mosaic.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_scatter.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/stubs/effect_trans_unique_tone.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/frame_warp.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/dynamic_shader.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/dyn_movement.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/dyn_distance.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/dyn_shift.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/movement.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/dynamic/zoom_rotate.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_blitter_feedback.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/parser.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/framebuffers.cpp
)

# Some stub sources are still TODO in the repository.  Filter out any entries
# that are referenced here but do not exist locally so CMake does not fail when
# new environments (like the Codespace container) have not generated them yet.
set(_avs_effect_stubs_filtered "")
foreach(_avs_effect_stub ${AVS_EFFECT_STUB_SOURCES})
  if(EXISTS "${_avs_effect_stub}")
    list(APPEND _avs_effect_stubs_filtered "${_avs_effect_stub}")
  else()
    message(STATUS "Skipping missing AVS effect stub: ${_avs_effect_stub}")
  endif()
endforeach()
set(AVS_EFFECT_STUB_SOURCES ${_avs_effect_stubs_filtered})
unset(_avs_effect_stubs_filtered)

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
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_blur_box.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_color_map.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_conv3x3.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_fast_brightness.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_grain.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/filters/effect_interferences.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_water.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_roto_blitter.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_mosaic.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_colorfade.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_color_clip.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_color_modifier.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_brightness.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_blur.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_water_bump.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_unique_tone.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_rotating_stars.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_simple_spectrum.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_oscilloscope_star.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_dot_plane.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_dot_fountain.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_bass_spin.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_moving_particle.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/misc/effect_render_mode.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/misc/effect_custom_bpm.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_multiplier.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_multi_delay.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_video_delay.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_color_reduction.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_scatter.cpp
  src/effects/primitive_dots.cpp
  src/effects/primitive_lines.cpp
  src/effects/primitive_rrect.cpp
  src/effects/primitive_solid.cpp
  src/effects/primitive_tri.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/render/effect_ring.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/misc/effect_comment.cpp
  src/text/text_renderer.cpp
  ${CMAKE_SOURCE_DIR}/src/effects/trans/effect_channel_shift.cpp
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
