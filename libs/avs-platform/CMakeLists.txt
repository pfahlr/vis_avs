find_package(PkgConfig REQUIRED)

include(FetchContent)

find_package(SDL2 2.28 QUIET)
if(NOT TARGET SDL2::SDL2)
  pkg_check_modules(SDL2 IMPORTED_TARGET sdl2>=2.28)
  if(SDL2_FOUND AND TARGET PkgConfig::SDL2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
  else()
    message(STATUS "Fetching SDL2 2.28.5")
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
      sdl2
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      URL https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz
      URL_HASH SHA256=332cb37d0be20cb9541739c61f79bae5a477427d79ae85e352089afdaf6666e4)
    FetchContent_MakeAvailable(sdl2)
    if(NOT TARGET SDL2::SDL2)
      if(TARGET SDL2::SDL2-static)
        add_library(SDL2::SDL2 ALIAS SDL2::SDL2-static)
      elseif(TARGET SDL2::SDL2-shared)
        add_library(SDL2::SDL2 ALIAS SDL2::SDL2-shared)
      else()
        message(FATAL_ERROR "SDL2 target not provided by FetchContent build")
      endif()
    endif()
  endif()
endif()
find_package(OpenGL REQUIRED)

find_package(PortAudio 19 QUIET)
if(NOT PortAudio_FOUND)
  FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG v19.7.0)
  FetchContent_MakeAvailable(portaudio)
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

if(PortAudio_FOUND)
  target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL PortAudio::PortAudio
                                            PkgConfig::SAMPLERATE)
else()
  target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL portaudio
                                            PkgConfig::SAMPLERATE)
endif()

