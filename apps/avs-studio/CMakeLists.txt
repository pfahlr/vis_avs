add_executable(avs-studio main.cpp)
target_link_libraries(avs-studio PRIVATE avs-core avs-core-runtime avs-effects-core avs-platform)
target_compile_options(avs-studio PRIVATE -Wall -Wextra -Werror)
