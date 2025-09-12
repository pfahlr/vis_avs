find_package(SDL2 REQUIRED)

add_executable(avs-player main.cpp)
target_link_libraries(avs-player PRIVATE avs-core avs-platform SDL2::SDL2main)
target_compile_options(avs-player PRIVATE -Wall -Wextra -Werror)
