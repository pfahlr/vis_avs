add_executable(gen_golden_md5
  gen_golden_md5.cpp)

target_link_libraries(gen_golden_md5 PRIVATE avs-offscreen)

target_compile_options(gen_golden_md5 PRIVATE -Wall -Wextra -Werror)

target_compile_definitions(gen_golden_md5 PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_compile_features(gen_golden_md5 PRIVATE cxx_std_20)

install(TARGETS gen_golden_md5
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

