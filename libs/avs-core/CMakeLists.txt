find_package(OpenGL REQUIRED)

add_library(avs-core)
add_library(avs::core ALIAS avs-core)
add_library(avs-core-runtime ALIAS avs-core)

set(AVS_CORE_HEADERS
  include/avs/core/DeterministicRng.hpp
  include/avs/core/EffectRegistry.hpp
  include/avs/core/IEffect.hpp
  include/avs/core/IFramebuffer.hpp
  include/avs/core/ParamBlock.hpp
  include/avs/core/Pipeline.hpp
  include/avs/core/RenderContext.hpp
  include/avs/core/ThreadPool.hpp
)

set(AVS_CORE_SOURCES
  src/CPUFramebuffer.cpp
  src/DeterministicRng.cpp
  src/EffectRegistry.cpp
  src/FileFramebuffer.cpp
  src/OpenGLFramebuffer.cpp
  src/Pipeline.cpp
  src/stb_image_write_impl.cpp
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
    OpenGL::GL
    Threads::Threads
)

target_compile_features(avs-core PUBLIC cxx_std_20)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)

target_include_directories(avs-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/third_party
)

install(TARGETS avs-core
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
