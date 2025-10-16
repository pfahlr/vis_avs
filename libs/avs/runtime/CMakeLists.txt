add_library(avs-runtime STATIC
  src/ResourceManager.cpp)

target_include_directories(avs-runtime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_options(avs-runtime PRIVATE -Wall -Wextra -Werror)

target_compile_definitions(avs-runtime
  PRIVATE
    AVS_RUNTIME_INSTALL_RESOURCES_DIR="${AVS_RESOURCES_INSTALL_FULL_DIR}"
    AVS_RUNTIME_BUILD_RESOURCES_DIR="${AVS_RESOURCES_BINARY_DIR}")

target_compile_features(avs-runtime PUBLIC cxx_std_20)

install(TARGETS avs-runtime
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
