cmake_minimum_required(VERSION 3.22)
project(avs-cross-platform VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AVS_INSTALL_RESOURCES "Install resources payload" ON)

set(AVS_RESOURCES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(AVS_RESOURCES_BINARY_DIR "${CMAKE_BINARY_DIR}/resources")
if(EXISTS "${AVS_RESOURCES_SOURCE_DIR}")
  add_custom_target(avs-resources ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${AVS_RESOURCES_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${AVS_RESOURCES_SOURCE_DIR}" "${AVS_RESOURCES_BINARY_DIR}"
    COMMENT "Syncing runtime resources")
  if(AVS_INSTALL_RESOURCES)
    install(DIRECTORY "${AVS_RESOURCES_SOURCE_DIR}/" DESTINATION share/vis_avs/resources)
  endif()
endif()

add_subdirectory(libs)
add_subdirectory(apps)
add_subdirectory(tools)
enable_testing()
add_subdirectory(tests)
