add_executable(gen_golden_md5
  gen_golden_md5.cpp)

target_link_libraries(gen_golden_md5 PRIVATE avs-offscreen)

target_compile_options(gen_golden_md5 PRIVATE -Wall -Wextra -Werror)

target_compile_definitions(gen_golden_md5 PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")

