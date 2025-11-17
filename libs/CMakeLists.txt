add_subdirectory(third_party/ns-eel)
add_subdirectory(avs-base)
add_subdirectory(avs-math)
add_subdirectory(avs-core)
add_subdirectory(avs-audio-io)
add_subdirectory(avs-audio-dsp)
add_subdirectory(avs-compat)
add_subdirectory(avs-dsl)
add_subdirectory(avs-render-gl)
add_subdirectory(avs-effects-legacy)
add_subdirectory(avs-effects-modern)
add_subdirectory(avs-preset)
add_subdirectory(avs-testing)

# Optional audio backend
if(UNIX AND NOT APPLE)
  add_subdirectory(avs-audio-pipewire)
endif()

add_library(avs-platform INTERFACE)
target_link_libraries(avs-platform INTERFACE
  avs::render-gl
  avs::audio-io
  avs::audio-dsp
  avs::base)
add_library(avs::platform ALIAS avs-platform)
