add_executable(avs-player main.cpp ${CMAKE_SOURCE_DIR}/libs/third_party/sha256.c)
target_include_directories(avs-player PRIVATE ${CMAKE_SOURCE_DIR}/libs/third_party)
target_link_libraries(avs-player PRIVATE avs-core avs-core-runtime avs-effects-core avs-platform avs-runtime avs-audio)
target_compile_options(avs-player PRIVATE -Wall -Wextra -Werror)
target_compile_features(avs-player PRIVATE cxx_std_20)
if(TARGET avs-resources)
  add_dependencies(avs-player avs-resources)
endif()

install(TARGETS avs-player
  EXPORT ${AVS_EXPORT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
