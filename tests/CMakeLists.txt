find_package(GTest REQUIRED)

add_executable(avs_core_tests avs_core_tests.cpp)
target_link_libraries(avs_core_tests PRIVATE avs-core GTest::gtest_main)
target_compile_options(avs_core_tests PRIVATE -Wall -Wextra -Werror)
add_test(NAME avs_core_tests COMMAND avs_core_tests)
