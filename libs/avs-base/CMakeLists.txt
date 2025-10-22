add_library(avs-base)
add_library(avs::base ALIAS avs-base)

set(AVS_BASE_HEADERS
  include/avs/base/fs.hpp
  include/avs/base/platform.hpp
  include/avs/fs.hpp
  include/avs/platform.hpp
)

set(AVS_BASE_SOURCES
  src/fs_posix.cpp
  src/platform.cpp
)

target_sources(avs-base
  PRIVATE
    ${AVS_BASE_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_BASE_HEADERS}
)

target_compile_features(avs-base PUBLIC cxx_std_20)
target_compile_options(avs-base PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-base PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-base
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
