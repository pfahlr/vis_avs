add_library(avs-core-runtime
  src/DeterministicRng.cpp
  src/EffectRegistry.cpp
  src/Pipeline.cpp
  src/effects/gating.cpp
  src/effects/transform_affine.cpp)

target_include_directories(avs-core-runtime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_options(avs-core-runtime PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-core-runtime PUBLIC cxx_std_20)

install(TARGETS avs-core-runtime
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
