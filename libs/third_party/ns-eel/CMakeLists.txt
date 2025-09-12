add_library(ns-eel STATIC
  nseel-compiler.c
  nseel-ram.c
  nseel-cfunc.c
  nseel-caltab.c
  nseel-lextab.c
  nseel-eval.c
  y.tab.c
)
target_include_directories(ns-eel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ns-eel PRIVATE EEL_TARGET_PORTABLE)
target_link_libraries(ns-eel PUBLIC m)
