find_package(PkgConfig REQUIRED)

if(ENABLE_SDL3)
  find_package(SDL3 QUIET)
  if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "ENABLE_SDL3 is ON but SDL3 was not found")
  endif()
  set(AVS_SDL_TARGET SDL3::SDL3)
else()
  find_package(SDL2 2.28 QUIET)
  if(NOT TARGET SDL2::SDL2)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=2.28)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
  endif()
  set(AVS_SDL_TARGET SDL2::SDL2)
endif()

find_package(OpenGL REQUIRED)

if(ENABLE_PORTAUDIO)
  find_package(PortAudio REQUIRED)
  set(AVS_PORTAUDIO_TARGET PortAudio::PortAudio)
else()
  find_package(PortAudio 19 QUIET)
  if(NOT PortAudio_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      portaudio
      GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
      GIT_TAG v19.7.0
      PATCH_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
        patch -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/patches/portaudio-warnings.patch)
    FetchContent_MakeAvailable(portaudio)
    set(AVS_PORTAUDIO_TARGET portaudio)
  else()
    set(AVS_PORTAUDIO_TARGET PortAudio::PortAudio)
  endif()
endif()

if(ENABLE_JACK)
  find_package(Jack QUIET)
  if(Jack_FOUND)
    set(AVS_JACK_TARGET Jack::Jack)
  endif()
endif()

if(ENABLE_FFTW3)
  find_package(FFTW3f QUIET)
endif()

if(ENABLE_FREETYPE)
  find_package(Freetype QUIET)
endif()

if(ENABLE_SDL_TTF)
  find_package(SDL_ttf QUIET)
endif()

if(ENABLE_STB_IMAGE)
  find_path(STB_IMAGE_INCLUDE_DIR stb_image.h)
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

target_include_directories(avs-platform PUBLIC include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/kissfft)
target_compile_options(avs-platform PRIVATE -Wall -Wextra -Werror)

target_link_libraries(avs-platform PUBLIC ${AVS_SDL_TARGET} OpenGL::GL ${AVS_PORTAUDIO_TARGET}
                                          PkgConfig::SAMPLERATE)

if(AVS_JACK_TARGET)
  target_link_libraries(avs-platform PUBLIC ${AVS_JACK_TARGET})
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_JACK=1)
endif()

if(ENABLE_FFTW3 AND FFTW3F_FOUND)
  target_link_libraries(avs-platform PUBLIC FFTW3::fftw3f)
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_FFTW3=1)
endif()

if(ENABLE_FREETYPE AND Freetype_FOUND)
  target_link_libraries(avs-platform PUBLIC Freetype::Freetype)
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_FREETYPE=1)
endif()

if(ENABLE_SDL_TTF AND SDL_ttf_FOUND)
  target_link_libraries(avs-platform PUBLIC SDL_ttf::SDL_ttf)
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_SDL_TTF=1)
endif()

if(ENABLE_STB_IMAGE AND STB_IMAGE_INCLUDE_DIR)
  target_include_directories(avs-platform PUBLIC ${STB_IMAGE_INCLUDE_DIR})
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_STB_IMAGE=1)
endif()

if(ENABLE_SDL3)
  target_compile_definitions(avs-platform PUBLIC AVS_USE_SDL3=1)
endif()

if(ENABLE_KISSFFT)
  target_compile_definitions(avs-platform PUBLIC AVS_HAVE_KISSFFT=1)
endif()

