include(FetchContent)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(avs_core_tests avs_core_tests.cpp)
target_link_libraries(avs_core_tests PRIVATE avs-core avs-platform GTest::gtest_main)
target_compile_options(avs_core_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(avs_core_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME avs_core_tests COMMAND avs_core_tests)

add_executable(deterministic_render_test deterministic_render_test.cpp)
target_link_libraries(deterministic_render_test PRIVATE GTest::gtest_main)
target_compile_options(deterministic_render_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(deterministic_render_test PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME deterministic_render_test COMMAND deterministic_render_test)
add_dependencies(deterministic_render_test avs-player)
