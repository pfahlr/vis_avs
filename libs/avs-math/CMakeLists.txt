add_library(avs-math INTERFACE)
add_library(avs::math ALIAS avs-math)

target_link_libraries(avs-math INTERFACE avs::base)

target_compile_features(avs-math INTERFACE cxx_std_20)

install(TARGETS avs-math EXPORT ${AVS_EXPORT_NAME})
