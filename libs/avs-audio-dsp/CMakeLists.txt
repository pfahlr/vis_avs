add_library(avs-audio-dsp)
add_library(avs::audio-dsp ALIAS avs-audio-dsp)

set(AVS_AUDIO_DSP_HEADERS
  include/avs/audio/analyzer.h
  include/avs/audio/fft.hpp
)

set(AVS_AUDIO_DSP_SOURCES
  src/analyzer.cpp
  src/fft_kiss.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/kiss_fft.c
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/kiss_fftr.c
)

target_sources(avs-audio-dsp
  PRIVATE
    ${AVS_AUDIO_DSP_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_AUDIO_DSP_HEADERS}
)

target_link_libraries(avs-audio-dsp
  PUBLIC
    avs::base
    avs::math
)

target_compile_features(avs-audio-dsp PUBLIC cxx_std_20)
target_compile_options(avs-audio-dsp PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-audio-dsp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/third_party>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-audio-dsp
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/kiss_fft.h
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/kiss_fft_log.h
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/kiss_fftr.h
  ${CMAKE_SOURCE_DIR}/libs/third_party/kissfft/_kiss_fft_guts.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/third_party/kissfft)
