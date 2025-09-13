add_executable(avs-studio
  main.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_draw.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_tables.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_widgets.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_impl_sdl2.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_impl_opengl3.cpp
  ${CMAKE_SOURCE_DIR}/libs/third_party/imgui/imgui_stdlib.cpp)
target_include_directories(avs-studio PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party/imgui)
target_link_libraries(avs-studio PRIVATE avs-core avs-platform)
target_compile_options(avs-studio PRIVATE -Wall -Wextra -Werror)
