add_executable(avs-player main.cpp ${CMAKE_SOURCE_DIR}/libs/third_party/sha256.c)
target_include_directories(avs-player PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party)
target_link_libraries(avs-player PRIVATE
  avs::base
  avs::math
  avs::core
  avs::audio-dsp
  avs::audio-io
  avs::compat
  avs::dsl
  avs::render-gl
  avs::effects-legacy
  avs::preset)
target_compile_options(avs-player PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-player PRIVATE cxx_std_20)
if(TARGET avs-resources)
  add_dependencies(avs-player avs-resources)
endif()

install(TARGETS avs-player
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
