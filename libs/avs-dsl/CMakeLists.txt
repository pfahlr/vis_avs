add_library(avs-dsl)
add_library(avs::dsl ALIAS avs-dsl)

set(AVS_DSL_HEADERS
  include/avs/runtime/script/eel_runtime.h
)

set(AVS_DSL_SOURCES
  src/script/eel_runtime.cpp
  src/script/eel_runtime.h
)

target_sources(avs-dsl
  PRIVATE
    ${AVS_DSL_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_DSL_HEADERS}
)

target_link_libraries(avs-dsl
  PUBLIC
    avs::core
    avs::compat
    ns-eel
)

target_compile_features(avs-dsl PUBLIC cxx_std_20)
target_compile_options(avs-dsl PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-dsl PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS avs-dsl
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
