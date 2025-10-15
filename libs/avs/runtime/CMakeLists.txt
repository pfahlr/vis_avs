add_library(avs-runtime STATIC
  src/ResourceManager.cpp)

target_include_directories(avs-runtime
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(avs-runtime PRIVATE -Wall -Wextra -Werror)

target_compile_definitions(avs-runtime
  PRIVATE
    AVS_RUNTIME_BUILD_RESOURCES_DIR="${CMAKE_BINARY_DIR}/resources"
    AVS_RUNTIME_INSTALL_RESOURCES_DIR="${CMAKE_INSTALL_PREFIX}/share/vis_avs/resources")
