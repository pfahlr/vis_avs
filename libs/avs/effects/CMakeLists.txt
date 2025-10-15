add_library(avs-effects-core
  src/Clear.cpp
  src/RegisterEffects.cpp
  src/Zoom.cpp)

target_link_libraries(avs-effects-core PUBLIC avs-core-runtime)

target_include_directories(avs-effects-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(avs-effects-core PRIVATE -Wall -Wextra -Werror)
