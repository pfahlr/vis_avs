find_package(PkgConfig REQUIRED)

find_package(PortAudio QUIET)
if(NOT PortAudio_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG v19.7.0
    PATCH_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR>
      patch -p1 --forward -i ${CMAKE_CURRENT_SOURCE_DIR}/patches/portaudio-warnings.patch || true)
  FetchContent_MakeAvailable(portaudio)
endif()

if(PortAudio_FOUND AND NOT TARGET PortAudio::PortAudio)
  if(DEFINED PORTAUDIO_LIBRARIES)
    add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
    set_target_properties(PortAudio::PortAudio PROPERTIES
      IMPORTED_LOCATION "${PORTAUDIO_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIRS}")
  endif()
elseif(TARGET portaudio)
  add_library(PortAudio::PortAudio ALIAS portaudio)
endif()

if(NOT TARGET PortAudio::PortAudio)
  pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
  add_library(PortAudio::PortAudio ALIAS PkgConfig::PORTAUDIO)
endif()

pkg_check_modules(SAMPLERATE REQUIRED IMPORTED_TARGET samplerate)

add_library(avs-audio-io)
add_library(avs::audio-io ALIAS avs-audio-io)
add_library(avs::audio ALIAS avs-audio-io)
add_library(avs-audio ALIAS avs-audio-io)

set(AVS_AUDIO_IO_HEADERS
  include/avs/audio/AudioEngine.hpp
  include/avs/audio/DeviceInfo.hpp
  include/avs/audio/audio.hpp
  include/avs/audio/audio_portaudio_internal.hpp
  include/avs/audio.hpp
  include/avs/audio_portaudio_internal.hpp
)

set(AVS_AUDIO_IO_SOURCES
  src/AudioEngine.cpp
  src/DeviceInfo.cpp
  src/audio_portaudio.cpp
)

target_sources(avs-audio-io
  PRIVATE
    ${AVS_AUDIO_IO_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_AUDIO_IO_HEADERS}
)

target_link_libraries(avs-audio-io
  PUBLIC
    avs::base
    avs::audio-dsp
    PortAudio::PortAudio
    PkgConfig::SAMPLERATE)

target_compile_features(avs-audio-io PUBLIC cxx_std_20)
target_compile_options(avs-audio-io PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-audio-io PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-audio-io
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
