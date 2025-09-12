find_package(SDL2 2.28 QUIET)
if(NOT SDL2_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=2.28)
  add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()
find_package(OpenGL REQUIRED)

find_package(PortAudio 19 QUIET)
if(NOT PortAudio_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG v19.7.0)
  FetchContent_MakeAvailable(portaudio)
endif()

add_library(avs-platform STATIC
  src/platform.cpp
  src/window_sdl_gl.cpp
  src/audio_portaudio.cpp
  src/fft_kiss.cpp
  ../third_party/kissfft/kiss_fft.c
  ../third_party/kissfft/kiss_fftr.c
)

target_include_directories(avs-platform PUBLIC include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/kissfft)
target_compile_options(avs-platform PRIVATE -Wall -Wextra -Werror)

if(PortAudio_FOUND)
  target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL PortAudio::PortAudio)
else()
  target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL portaudio)
endif()

