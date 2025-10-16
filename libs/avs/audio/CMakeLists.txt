find_package(PortAudio QUIET)
if(NOT TARGET PortAudio::PortAudio)
  if(PortAudio_FOUND AND DEFINED PORTAUDIO_LIBRARIES)
    add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
    set_target_properties(PortAudio::PortAudio PROPERTIES
      IMPORTED_LOCATION "${PORTAUDIO_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIRS}")
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO QUIET IMPORTED_TARGET portaudio-2.0)
    if(TARGET PkgConfig::PORTAUDIO)
      add_library(PortAudio::PortAudio ALIAS PkgConfig::PORTAUDIO)
    else()
      pkg_check_modules(PORTAUDIO QUIET IMPORTED_TARGET portaudio)
      if(TARGET PkgConfig::PORTAUDIO)
        add_library(PortAudio::PortAudio ALIAS PkgConfig::PORTAUDIO)
      endif()
    endif()
  endif()
endif()

if(NOT TARGET PortAudio::PortAudio)
  message(FATAL_ERROR
    "PortAudio not found. Install portaudio development files or provide a CMake package.")
endif()

add_library(avs-audio
  AudioEngine.cpp
  DeviceInfo.cpp)

target_include_directories(avs-audio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(avs-audio PUBLIC PortAudio::PortAudio)

target_compile_options(avs-audio PRIVATE -Wall -Wextra -Werror)

target_compile_features(avs-audio PUBLIC cxx_std_20)

install(TARGETS avs-audio
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
  AudioEngine.hpp
  DeviceInfo.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/avs/audio)
