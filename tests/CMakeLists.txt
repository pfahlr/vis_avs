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

add_executable(audio_underflow_tests audio_underflow_tests.cpp)
target_link_libraries(audio_underflow_tests PRIVATE avs-platform GTest::gtest_main)
target_compile_options(audio_underflow_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME audio_underflow_tests COMMAND audio_underflow_tests)

add_executable(deterministic_render_test deterministic_render_test.cpp)
target_link_libraries(deterministic_render_test PRIVATE GTest::gtest_main)
target_compile_options(deterministic_render_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(deterministic_render_test PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME deterministic_render_test COMMAND deterministic_render_test)
add_dependencies(deterministic_render_test avs-player)

add_executable(core_effects_tests
  core/test_effect_registry.cpp
  core/test_pipeline.cpp)
target_link_libraries(core_effects_tests PRIVATE avs-effects-core avs-core-runtime GTest::gtest_main)
target_compile_options(core_effects_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME core_effects_tests COMMAND core_effects_tests)

add_executable(runtime_resource_tests
  runtime/test_resources.cpp)
target_link_libraries(runtime_resource_tests PRIVATE avs-runtime GTest::gtest_main)
target_compile_options(runtime_resource_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(runtime_resource_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME runtime_resource_tests COMMAND runtime_resource_tests)
if(TARGET avs-resources)
  add_dependencies(runtime_resource_tests avs-resources)
endif()

set(PHASE1_DATA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/data/phase1")
set(PHASE1_GOLDEN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/golden/phase1")
set(PHASE1_DATA_DEST "${CMAKE_BINARY_DIR}/tests/data/phase1")
set(PHASE1_GOLDEN_DEST "${CMAKE_BINARY_DIR}/tests/golden/phase1")

if(EXISTS "${PHASE1_DATA_SOURCE}" AND EXISTS "${PHASE1_GOLDEN_SOURCE}")
  add_custom_command(
    TARGET deterministic_render_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/tests/data"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/tests/golden"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${PHASE1_DATA_DEST}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${PHASE1_GOLDEN_DEST}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PHASE1_DATA_SOURCE}" "${PHASE1_DATA_DEST}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PHASE1_GOLDEN_SOURCE}" "${PHASE1_GOLDEN_DEST}"
    COMMENT "Copying deterministic render phase1 assets to build tree")
else()
  message(WARNING "Phase1 deterministic render assets are missing; deterministic_render_test may fail")
endif()
