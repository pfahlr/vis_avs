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
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(avs-offscreen
  PUBLIC avs-core avs-platform)

target_compile_options(avs-offscreen PRIVATE -Wall -Wextra -Werror)

install(TARGETS avs-offscreen
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
  OffscreenRenderer.hpp
  Md5.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/avs/offscreen)
