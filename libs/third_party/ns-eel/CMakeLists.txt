add_library(ns-eel STATIC
  nseel-compiler.c
  nseel-ram.c
  nseel-cfunc.c
  nseel-caltab.c
  nseel-lextab.c
  nseel-eval.c
  y.tab.c
)
target_include_directories(ns-eel
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/third_party/ns-eel>)
target_compile_definitions(ns-eel PRIVATE EEL_TARGET_PORTABLE)
target_link_libraries(ns-eel PUBLIC m)

install(TARGETS ns-eel
  EXPORT ${AVS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
  glue_port.h
  ns-eel-addfuncs.h
  ns-eel.h
  ns-eel-int.h
  nseel_lexer_decls.h
  utf8_extended.h
  wdlcstring.h
  wdltypes.h
  denormal.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/third_party/ns-eel)
