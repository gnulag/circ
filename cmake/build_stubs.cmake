file(GLOB_RECURSE stubs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/scheme_libs
  ${CMAKE_CURRENT_SOURCE_DIR}/scheme_libs/*.stub)

set(chibi-ffi ${CMAKE_CURRENT_SOURCE_DIR}/chibi-scheme/tools/chibi-ffi)
set(chibi-genstatic ${CMAKE_CURRENT_SOURCE_DIR}/chibi-scheme/tools/chibi-genstatic)

set(stuboutdir ${CMAKE_CURRENT_BINARY_DIR}/scheme_libs/)
foreach(e ${stubs})
  get_filename_component(stubdir ${e} PATH)
  get_filename_component(basename ${e} NAME_WE)
  set(stubfile ${CMAKE_CURRENT_SOURCE_DIR}/scheme_libs/${e})
  set(stubdir ${stuboutdir}/${stubdir})
  set(stubout ${stubdir}/${basename}.c)
  set(libout ${stubdir}/${basename}.so)
  file(MAKE_DIRECTORY ${stubdir})
  add_custom_command(OUTPUT ${stubout}
    COMMAND chibi-scheme ${chibi-ffi} ${stubfile} ${stubout}
    DEPENDS ${stubfile} ${chibi-ffi}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_command(OUTPUT ${libout}
    COMMAND cc -fPIC -shared ${stubout} -o ${libout}
    DEPENDS ${stubout}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  list(APPEND stubouts ${stubout})
  list(APPEND libouts ${libout})
endforeach()

add_custom_target(build_stubs DEPENDS ${stubouts} ${libouts})
