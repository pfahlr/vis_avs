if(ENABLE_PORTAUDIO AND TARGET avs-platform)
  add_subdirectory(avs-player)
endif()

if(TARGET avs-platform)
  add_subdirectory(avs-studio)
endif()
