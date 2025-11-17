cmake_minimum_required(VERSION 3.22)
project(vis_avs VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED ENV{AVS_SKIP_AUTO_DEPS})
  set(_avs_auto_dep_script "${CMAKE_SOURCE_DIR}/tools/ensure_build_dependencies.sh")
  if(EXISTS "${_avs_auto_dep_script}")
    execute_process(
      COMMAND "${_avs_auto_dep_script}" "${CMAKE_BINARY_DIR}"
      RESULT_VARIABLE _avs_auto_dep_result
      OUTPUT_VARIABLE _avs_auto_dep_stdout
      ERROR_VARIABLE _avs_auto_dep_stderr
      COMMAND_ECHO STDOUT)
    if(NOT _avs_auto_dep_result EQUAL 0)
      message(STATUS "Automatic dependency bootstrap output:\n${_avs_auto_dep_stdout}")
      message(STATUS "Automatic dependency bootstrap errors:\n${_avs_auto_dep_stderr}")
      message(FATAL_ERROR "Failed to ensure build dependencies. Set AVS_SKIP_AUTO_DEPS=1 to skip automatic installation and install prerequisites manually by running run_setup_dev_environment.sh.")
    endif()
  endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_INSTALL_INCLUDEDIR "include/vis_avs" CACHE PATH "" FORCE)
set(CMAKE_INSTALL_LIBDIR "lib/vis_avs" CACHE PATH "" FORCE)

option(AVS_BUILD_PLAYER "Build the avs-player application" ON)
option(AVS_BUILD_TOOLS "Build auxiliary tooling" ON)
option(AVS_ENABLE_SDL3 "Enable the experimental SDL3 backend" OFF)
option(AVS_INSTALL_RESOURCES "Install resources payload" ON)
option(AVS_BUILD_AUDIO "Build PortAudio-backed runtime audio" ON)

set(AVS_EXPORT_NAME vis_avsTargets)

set(AVS_RESOURCES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(AVS_RESOURCES_BINARY_DIR "${CMAKE_BINARY_DIR}/resources")
set(AVS_RESOURCES_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/vis_avs/resources")
set(AVS_RESOURCES_INSTALL_FULL_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/vis_avs/resources")

if(EXISTS "${AVS_RESOURCES_SOURCE_DIR}")
  add_custom_target(avs-resources ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${AVS_RESOURCES_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${AVS_RESOURCES_SOURCE_DIR}" "${AVS_RESOURCES_BINARY_DIR}"
    COMMENT "Syncing runtime resources")
  if(AVS_INSTALL_RESOURCES)
    install(DIRECTORY "${AVS_RESOURCES_SOURCE_DIR}/" DESTINATION "${AVS_RESOURCES_INSTALL_DIR}")
  endif()
endif()

add_subdirectory(libs)
add_subdirectory(apps)

if(AVS_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

enable_testing()
add_subdirectory(tests)

install(EXPORT ${AVS_EXPORT_NAME}
  NAMESPACE vis_avs::
  FILE vis_avsTargets.cmake
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vis_avs")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vis_avsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vis_avsConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vis_avs"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/vis_avsConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/vis_avsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/vis_avsConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/vis_avs")
