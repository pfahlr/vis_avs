add_library(avs-core-runtime
  src/DeterministicRng.cpp
  src/EffectRegistry.cpp
  src/Pipeline.cpp)

target_include_directories(avs-core-runtime
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(avs-core-runtime PRIVATE -Wall -Wextra -Werror)
