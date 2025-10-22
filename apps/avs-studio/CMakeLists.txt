add_executable(avs-studio main.cpp)
target_link_libraries(avs-studio PRIVATE
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
target_compile_options(avs-studio PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-studio PRIVATE cxx_std_20)

install(TARGETS avs-studio
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
