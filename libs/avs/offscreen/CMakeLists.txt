add_library(avs-offscreen
  OffscreenRenderer.cpp
  Md5.cpp)

if(NOT TARGET avs-platform)
  message(FATAL_ERROR "avs-platform target is required for avs-offscreen")
endif()

if(NOT TARGET avs-core)
  message(FATAL_ERROR "avs-core target is required for avs-offscreen")
endif()

target_include_directories(avs-offscreen
  PUBLIC ${CMAKE_SOURCE_DIR}/libs
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(avs-offscreen
  PUBLIC avs-core avs-platform)

target_compile_options(avs-offscreen PRIVATE -Wall -Wextra -Werror)

