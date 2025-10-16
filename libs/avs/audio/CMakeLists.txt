find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio_static)

if(NOT PORTAUDIO_INCLUDE_DIR OR NOT PORTAUDIO_LIBRARY)
  message(FATAL_ERROR
          "PortAudio not found. Install portaudio19-dev/libportaudio2 (or provide PORTAUDIO_INCLUDE_DIR/PORTAUDIO_LIBRARY).")
endif()

if(NOT TARGET PortAudio::PortAudio)
  add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
  set_target_properties(PortAudio::PortAudio PROPERTIES
    IMPORTED_LOCATION "${PORTAUDIO_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIR}")
endif()

add_library(avs-audio
  AudioEngine.cpp
  DeviceInfo.cpp)

target_include_directories(avs-audio PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(avs-audio PUBLIC PortAudio::PortAudio)

target_compile_options(avs-audio PRIVATE -Wall -Wextra -Werror)
