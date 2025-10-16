find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

set(AVS_PLATFORM_SDL_TARGET "")
if(AVS_ENABLE_SDL3)
  find_package(SDL3 QUIET)
  if(TARGET SDL3::SDL3)
    set(AVS_PLATFORM_SDL_TARGET SDL3::SDL3)
  else()
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    if(TARGET PkgConfig::SDL3)
      add_library(SDL3::SDL3 ALIAS PkgConfig::SDL3)
      set(AVS_PLATFORM_SDL_TARGET SDL3::SDL3)
    endif()
  endif()
else()
  set(SDL2_MIN_VERSION 2.0.20)
  find_package(SDL2 ${SDL2_MIN_VERSION} QUIET)
  if(TARGET SDL2::SDL2)
    set(AVS_PLATFORM_SDL_TARGET SDL2::SDL2)
  else()
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=${SDL2_MIN_VERSION})
    if(TARGET PkgConfig::SDL2)
      add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
      set(AVS_PLATFORM_SDL_TARGET SDL2::SDL2)
    endif()
  endif()
endif()

if(NOT AVS_PLATFORM_SDL_TARGET)
  message(FATAL_ERROR "Failed to locate the required SDL dependency")
endif()

find_package(PortAudio 19 QUIET)
if(PortAudio_FOUND)
  if(NOT TARGET PortAudio::PortAudio)
    if(DEFINED PORTAUDIO_LIBRARIES)
      add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
      set_target_properties(PortAudio::PortAudio PROPERTIES
        IMPORTED_LOCATION "${PORTAUDIO_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIRS}")
    endif()
  endif()
else()
  include(FetchContent)
  FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG v19.7.0
    PATCH_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
      patch -p1 --forward -i ${CMAKE_CURRENT_SOURCE_DIR}/patches/portaudio-warnings.patch || true)
  FetchContent_MakeAvailable(portaudio)
endif()

if(NOT TARGET PortAudio::PortAudio)
  if(TARGET portaudio)
    add_library(PortAudio::PortAudio ALIAS portaudio)
  else()
    pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
    add_library(PortAudio::PortAudio ALIAS PkgConfig::PORTAUDIO)
  endif()
endif()

pkg_check_modules(SAMPLERATE REQUIRED IMPORTED_TARGET samplerate)

add_library(avs-platform STATIC
  src/platform.cpp
  src/window_sdl_gl.cpp
  src/audio_portaudio.cpp
  src/fft_kiss.cpp
  src/fs_posix.cpp
  ../third_party/kissfft/kiss_fft.c
  ../third_party/kissfft/kiss_fftr.c
)

target_include_directories(avs-platform
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/kissfft)
target_compile_options(avs-platform PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-platform PUBLIC cxx_std_20)

target_link_libraries(avs-platform PUBLIC
  ${AVS_PLATFORM_SDL_TARGET}
  OpenGL::GL
  PortAudio::PortAudio
  PkgConfig::SAMPLERATE)

install(TARGETS avs-platform
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES
  ../third_party/kissfft/kiss_fft.h
  ../third_party/kissfft/kiss_fft_log.h
  ../third_party/kissfft/kiss_fftr.h
  ../third_party/kissfft/_kiss_fft_guts.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/third_party/kissfft)

