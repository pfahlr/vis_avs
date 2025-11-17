# Pipewire audio backend (Linux only)
# Requires libpipewire-0.3-dev for full implementation

option(AVS_ENABLE_PIPEWIRE "Enable Pipewire audio backend (Linux only)" ON)

if(NOT AVS_ENABLE_PIPEWIRE OR NOT UNIX OR APPLE)
  message(STATUS "Pipewire audio backend disabled (only available on Linux)")
  return()
endif()

# Check for Pipewire (optional dependency)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PIPEWIRE libpipewire-0.3)
endif()

if(NOT PIPEWIRE_FOUND)
  message(STATUS "Pipewire not found - building stub implementation only")
  message(STATUS "Install libpipewire-0.3-dev for full Pipewire support")
endif()

add_library(avs-audio-pipewire)
add_library(avs::audio-pipewire ALIAS avs-audio-pipewire)

set(AVS_AUDIO_PIPEWIRE_HEADERS
  include/avs/audio/pipewire_input.h
)

set(AVS_AUDIO_PIPEWIRE_SOURCES
  src/pipewire_input.cpp
)

target_sources(avs-audio-pipewire
  PRIVATE
    ${AVS_AUDIO_PIPEWIRE_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_AUDIO_PIPEWIRE_HEADERS}
)

target_include_directories(avs-audio-pipewire PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(PIPEWIRE_FOUND)
  target_link_libraries(avs-audio-pipewire PRIVATE ${PIPEWIRE_LIBRARIES})
  target_include_directories(avs-audio-pipewire PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
  target_compile_definitions(avs-audio-pipewire PRIVATE AVS_HAS_PIPEWIRE=1)
endif()

target_compile_features(avs-audio-pipewire PUBLIC cxx_std_20)
target_compile_options(avs-audio-pipewire PRIVATE -Wall -Wextra -Werror)

install(TARGETS avs-audio-pipewire
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
