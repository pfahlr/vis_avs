add_executable(avs-player main.cpp)
target_link_libraries(avs-player PRIVATE avs-core avs-platform)
target_compile_options(avs-player PRIVATE -Wall -Wextra -Werror)
