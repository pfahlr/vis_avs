add_library(avs-preset)
add_library(avs::preset ALIAS avs-preset)

set(AVS_PRESET_HEADERS
  include/avs/preset/binder.h
  include/avs/preset/ir.h
  include/avs/preset/parser.h
)

set(AVS_PRESET_SOURCES
  src/binder.cpp
  src/parser.cpp
)

target_sources(avs-preset
  PRIVATE
    ${AVS_PRESET_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_PRESET_HEADERS}
)

target_link_libraries(avs-preset
  PUBLIC
    avs::effects-api
    avs::core
    avs::dsl
)

target_compile_features(avs-preset PUBLIC cxx_std_20)
target_compile_options(avs-preset PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-preset PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-preset
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
