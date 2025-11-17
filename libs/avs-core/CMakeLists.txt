add_library(avs-core)
add_library(avs::core ALIAS avs-core)
add_library(avs-core-runtime ALIAS avs-core)

set(AVS_CORE_HEADERS
  include/avs/core/DeterministicRng.hpp
  include/avs/core/EffectRegistry.hpp
  include/avs/core/IEffect.hpp
  include/avs/core/ParamBlock.hpp
  include/avs/core/Pipeline.hpp
  include/avs/core/RenderContext.hpp
  include/avs/core/ThreadPool.hpp
)

set(AVS_CORE_SOURCES
  src/DeterministicRng.cpp
  src/EffectRegistry.cpp
  src/Pipeline.cpp
  src/ThreadPool.cpp
)

target_sources(avs-core
  PRIVATE
    ${AVS_CORE_SOURCES}
  PUBLIC
    FILE_SET public_headers
      TYPE HEADERS
      BASE_DIRS include
      FILES ${AVS_CORE_HEADERS}
)

target_link_libraries(avs-core
  PUBLIC
    avs::base
    avs::math
  PRIVATE
    Threads::Threads
)

target_compile_features(avs-core PUBLIC cxx_std_20)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(TARGETS avs-core
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
