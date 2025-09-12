find_package(SDL2 2.28 QUIET)
if(NOT SDL2_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2>=2.28)
  add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()
find_package(OpenGL REQUIRED)

add_library(avs-platform STATIC
  src/platform.cpp
  src/window_sdl_gl.cpp
)
target_include_directories(avs-platform PUBLIC include)
target_link_libraries(avs-platform PUBLIC SDL2::SDL2 OpenGL::GL)
