cmake_minimum_required(VERSION 3.22)
project(avs-cross-platform VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(libs)
add_subdirectory(apps)
enable_testing()
add_subdirectory(tests)
