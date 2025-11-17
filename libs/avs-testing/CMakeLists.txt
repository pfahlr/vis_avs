add_library(avs-testing)
add_library(avs::testing ALIAS avs-testing)
add_library(avs::offscreen ALIAS avs-testing)
add_library(avs-offscreen ALIAS avs-testing)

set(AVS_TESTING_HEADERS
  include/avs/testing/Md5.hpp
  include/avs/testing/OffscreenRenderer.hpp
  include/avs/offscreen/Md5.hpp
  include/avs/offscreen/OffscreenRenderer.hpp
)

set(AVS_TESTING_SOURCES
  src/Md5.cpp
  src/OffscreenRenderer.cpp
)

target_sources(avs-testing
  PRIVATE
    ${AVS_TESTING_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_TESTING_HEADERS}
)

target_link_libraries(avs-testing
  PUBLIC
    avs::core
    avs::audio-io
    avs::audio-dsp
    avs::compat
    avs::effects-legacy
    avs::preset
    avs::render-gl
)

target_compile_features(avs-testing PUBLIC cxx_std_20)
target_compile_options(avs-testing PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-testing PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-testing
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
