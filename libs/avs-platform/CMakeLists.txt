find_package(SDL2 2.28 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(avs-platform STATIC
  src/platform.cpp
  src/window_sdl_gl.cpp
)
target_include_directories(avs-platform PUBLIC include)
target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL)
