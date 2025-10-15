add_executable(avs-player main.cpp ${CMAKE_SOURCE_DIR}/libs/third_party/sha256.c)
target_include_directories(avs-player PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party)
target_link_libraries(avs-player PRIVATE avs-core avs-core-runtime avs-effects-core avs-platform)
target_compile_options(avs-player PRIVATE -Wall -Wextra -Werror)
