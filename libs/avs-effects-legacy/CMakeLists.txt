file(GLOB_RECURSE AVS_EFFECTS_LEGACY_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE AVS_EFFECTS_LEGACY_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/avs/effects/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/avs/effects/*.h")

add_library(avs-effects-api INTERFACE)
add_library(avs::effects-api ALIAS avs-effects-api)
target_include_directories(avs-effects-api INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(avs-effects-legacy)
add_library(avs::effects-legacy ALIAS avs-effects-legacy)
add_library(avs-effects-core ALIAS avs-effects-legacy)

target_sources(avs-effects-legacy
  PRIVATE
    ${AVS_EFFECTS_LEGACY_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_EFFECTS_LEGACY_HEADERS}
)

target_link_libraries(avs-effects-legacy
  PUBLIC
    avs::effects-api
    avs::core
    avs::dsl
    avs::compat
    avs::render-gl
)

target_compile_features(avs-effects-legacy PUBLIC cxx_std_20)
target_compile_options(avs-effects-legacy PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-effects-legacy PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

install(TARGETS avs-effects-legacy
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS avs-effects-api
  EXPORT ${AVS_EXPORT_NAME}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
