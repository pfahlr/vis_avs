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
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(avs_core_tests avs_core_tests.cpp)
target_link_libraries(avs_core_tests PRIVATE avs-core avs-platform GTest::gtest_main)
target_compile_options(avs_core_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(avs_core_tests PRIVATE
  BUILD_DIR="${CMAKE_BINARY_DIR}"
  SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  AVS_BUILD_AUDIO=$<BOOL:${AVS_BUILD_AUDIO}>
  AVS_BUILD_PLATFORM=$<BOOL:${AVS_BUILD_PLATFORM}>)

target_include_directories(avs_core_tests PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party ${CMAKE_SOURCE_DIR}/src)

add_test(NAME avs_core_tests COMMAND avs_core_tests)

if(AVS_BUILD_AUDIO)
  add_executable(audio_underflow_tests audio_underflow_tests.cpp)
  target_link_libraries(audio_underflow_tests PRIVATE avs-platform GTest::gtest_main)
  target_compile_options(audio_underflow_tests PRIVATE -Wall -Wextra -Werror)
  add_test(NAME audio_underflow_tests COMMAND audio_underflow_tests)

  add_executable(audio_device_negotiation_tests audio/test_device_negotiation.cpp)
  target_link_libraries(audio_device_negotiation_tests PRIVATE avs-audio GTest::gtest_main)
  target_compile_options(audio_device_negotiation_tests PRIVATE -Wall -Wextra -Werror)
  add_test(NAME audio_device_negotiation_tests COMMAND audio_device_negotiation_tests)

  add_executable(audio_analyzer_tests audio/test_analyzer.cpp)
  target_link_libraries(audio_analyzer_tests PRIVATE avs-platform avs-audio GTest::gtest_main)
  target_compile_options(audio_analyzer_tests PRIVATE -Wall -Wextra -Werror)
  target_include_directories(audio_analyzer_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
  add_test(NAME audio_analyzer_tests COMMAND audio_analyzer_tests)
endif()

add_executable(deterministic_render_test deterministic_render_test.cpp)
target_link_libraries(deterministic_render_test PRIVATE GTest::gtest_main)
target_compile_options(deterministic_render_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(deterministic_render_test PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME deterministic_render_test COMMAND deterministic_render_test)
if(TARGET avs-player)
  add_dependencies(deterministic_render_test avs-player)
endif()

add_executable(core_effects_tests
  core/test_effect_registry.cpp
  core/test_pipeline.cpp
  core/test_blend_ops.cpp
  core/test_scripted_effect.cpp
  core/test_globals_and_bump.cpp
  core/test_transform_affine.cpp
  core/test_gating.cpp
  core/test_primitives.cpp
  core/test_multi_delay.cpp)

target_link_libraries(core_effects_tests PRIVATE avs-effects-core avs-core-runtime avs-offscreen GTest::gtest_main)
target_compile_options(core_effects_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(core_effects_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(core_effects_tests PRIVATE ${CMAKE_SOURCE_DIR}/libs/avs/effects/src)
add_test(NAME core_effects_tests COMMAND core_effects_tests)

if(AVS_BUILD_AUDIO)
  add_executable(audio_vis_tests
    presets/audio_vis/test_audio_vis.cpp)
  target_link_libraries(audio_vis_tests PRIVATE avs-effects-core avs-core-runtime avs-offscreen GTest::gtest_main)
  target_compile_options(audio_vis_tests PRIVATE -Wall -Wextra -Werror)
  target_compile_definitions(audio_vis_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_test(NAME audio_vis_tests COMMAND audio_vis_tests)
endif()

add_executable(dynamic_effects_tests
  presets/dynamic/test_dynamic.cpp
  presets/fb_ops/md5_helper.cpp)
target_link_libraries(dynamic_effects_tests PRIVATE avs-effects-core avs-core-runtime avs-offscreen GTest::gtest_main)
target_compile_options(dynamic_effects_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(dynamic_effects_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(dynamic_effects_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests/presets/fb_ops)
add_test(NAME dynamic_effects_tests COMMAND dynamic_effects_tests)

add_executable(filter_effects_tests
  presets/filters/test_filters.cpp)
target_link_libraries(filter_effects_tests PRIVATE avs-effects-core avs-core-runtime avs-offscreen GTest::gtest_main)
target_compile_options(filter_effects_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(filter_effects_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME filter_effects_tests COMMAND filter_effects_tests)

add_executable(runtime_resource_tests
  runtime/test_resources.cpp)
target_link_libraries(runtime_resource_tests PRIVATE avs-runtime GTest::gtest_main)
target_compile_options(runtime_resource_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(runtime_resource_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME runtime_resource_tests COMMAND runtime_resource_tests)
if(TARGET avs-resources)
  add_dependencies(runtime_resource_tests avs-resources)
endif()

add_executable(runtime_effect_list_parser_tests
  runtime/parser/effect_list_parser_tests.cpp)
target_link_libraries(runtime_effect_list_parser_tests PRIVATE avs-core avs-runtime GTest::gtest_main)
target_compile_options(runtime_effect_list_parser_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(runtime_effect_list_parser_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME runtime_effect_list_parser_tests COMMAND runtime_effect_list_parser_tests)


if(AVS_BUILD_AUDIO)
  add_executable(offscreen_golden_test
    regression/test_offscreen_golden.cpp)
  target_link_libraries(offscreen_golden_test PRIVATE avs-offscreen GTest::gtest_main)
  target_compile_options(offscreen_golden_test PRIVATE -Wall -Wextra -Werror)
  target_compile_definitions(offscreen_golden_test PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_test(NAME offscreen_golden_test COMMAND offscreen_golden_test)
endif()

add_executable(runtime_compositing_tests
  runtime/test_runtime_compositing.cpp)
target_link_libraries(runtime_compositing_tests PRIVATE avs-core avs-runtime GTest::gtest_main)
target_compile_options(runtime_compositing_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(runtime_compositing_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(runtime_compositing_tests PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party ${CMAKE_SOURCE_DIR}/src)
add_test(NAME runtime_compositing_tests COMMAND runtime_compositing_tests)



add_executable(fb_ops_tests
  presets/fb_ops/test_fb_ops.cpp
  presets/fb_ops/md5_helper.cpp)
target_link_libraries(fb_ops_tests PRIVATE avs-runtime GTest::gtest_main)
target_compile_options(fb_ops_tests PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(fb_ops_tests PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME fb_ops_tests COMMAND fb_ops_tests)

if(TARGET gen_golden_md5)
  add_test(NAME offscreen_golden_md5_snapshot
    COMMAND Python3::Interpreter
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_golden_md5.py
            --binary $<TARGET_FILE:gen_golden_md5>
            --expected ${CMAKE_CURRENT_SOURCE_DIR}/regression/data/expected_md5_320x240_seed1234.json
            --frames 10 --width 320 --height 240 --seed 1234
            --preset ${CMAKE_SOURCE_DIR}/tests/regression/data/tiny_preset_fragment.avs)
else()
  message(WARNING "Golden MD5 snapshot test disabled because gen_golden_md5 is not built")
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
