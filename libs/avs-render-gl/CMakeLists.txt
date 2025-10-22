find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

set(AVS_RENDER_GL_SDL_TARGET "")
if(AVS_ENABLE_SDL3)
  find_package(SDL3 QUIET)
  if(TARGET SDL3::SDL3)
    set(AVS_RENDER_GL_SDL_TARGET SDL3::SDL3)
  else()
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    if(TARGET PkgConfig::SDL3)
      add_library(SDL3::SDL3 ALIAS PkgConfig::SDL3)
      set(AVS_RENDER_GL_SDL_TARGET SDL3::SDL3)
    endif()
  endif()
else()
  set(SDL2_MIN_VERSION 2.0.20)
  find_package(SDL2 ${SDL2_MIN_VERSION} QUIET)
  if(TARGET SDL2::SDL2)
    set(AVS_RENDER_GL_SDL_TARGET SDL2::SDL2)
  else()
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=${SDL2_MIN_VERSION})
    if(TARGET PkgConfig::SDL2)
      add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
      set(AVS_RENDER_GL_SDL_TARGET SDL2::SDL2)
    endif()
  endif()
endif()

if(NOT AVS_RENDER_GL_SDL_TARGET)
  message(FATAL_ERROR "Failed to locate the required SDL dependency")
endif()

add_library(avs-render-gl)
add_library(avs::render-gl ALIAS avs-render-gl)

set(AVS_RENDER_GL_HEADERS
  include/avs/render/gl/window.hpp
  include/avs/window.hpp
)

set(AVS_RENDER_GL_SOURCES
  src/window_sdl_gl.cpp
)

target_sources(avs-render-gl
  PRIVATE
    ${AVS_RENDER_GL_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_RENDER_GL_HEADERS}
)

target_link_libraries(avs-render-gl
  PUBLIC
    avs::core
    ${AVS_RENDER_GL_SDL_TARGET}
    OpenGL::GL
)

target_compile_features(avs-render-gl PUBLIC cxx_std_20)
target_compile_options(avs-render-gl PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-render-gl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-render-gl
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
