add_library(avs-effects-modern INTERFACE)
add_library(avs::effects-modern ALIAS avs-effects-modern)

target_link_libraries(avs-effects-modern INTERFACE
  avs::core
  avs::audio-dsp
  avs::render-gl
)

target_compile_features(avs-effects-modern INTERFACE cxx_std_20)

install(TARGETS avs-effects-modern EXPORT ${AVS_EXPORT_NAME})
