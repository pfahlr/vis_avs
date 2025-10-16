add_library(avs-effects-core
  src/Blend.cpp
  src/Clear.cpp
  src/Overlay.cpp
  src/Text.cpp
  src/RegisterEffects.cpp
  src/Swizzle.cpp
  src/Zoom.cpp
  src/micro_preset_parser.cpp
  src/effects/blend_ops.cpp
  src/effects/primitive_dots.cpp
  src/effects/primitive_lines.cpp
  src/effects/primitive_rrect.cpp
  src/effects/primitive_solid.cpp
  src/effects/primitive_tri.cpp
  src/text/text_renderer.cpp)

target_link_libraries(avs-effects-core PUBLIC avs-core-runtime)

target_include_directories(avs-effects-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
