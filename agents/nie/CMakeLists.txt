
include(NepomukMacros)

# FIXME: Make this macro take a list of ontology files and then move it to NepomukMacros.cmake
macro(NEPOMUK_GENERATE_FROM_ONTOLOGY2 ontofile1 ontofile2 ontofile3 ontofile4 targetdir out_headers out_sources out_includes)

  # init
  set(NEPOMUK_RESOURCES_GENERATED false)

  FIND_PROGRAM(RCGEN nepomuk-rcgen PATHS ${BIN_INSTALL_DIR})

  if(NOT RCGEN)

    message(STATUS "Failed to find the Nepomuk source generator" )

  else(NOT RCGEN)

    FILE(TO_NATIVE_PATH ${RCGEN} RCGEN)

    execute_process(
      COMMAND ${RCGEN} --listheaders --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
      OUTPUT_VARIABLE ${out_headers}
      RESULT_VARIABLE rcgen_result
      ERROR_QUIET
      )

    # If the first call succeeds it is very very likely that the rest will, too
    if(${rcgen_result} EQUAL 0)

      execute_process(
        COMMAND ${RCGEN} --listsources --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
        OUTPUT_VARIABLE ${out_sources}
        ERROR_QUIET
        )

      execute_process(
        COMMAND ${RCGEN} --listincludes --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
        OUTPUT_VARIABLE ${out_includes}
        ERROR_QUIET
        )

      execute_process(
        COMMAND ${RCGEN} --writeall --templates ${ARGN} --target ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
        ERROR_QUIET
        )

      set(NEPOMUK_RESOURCES_GENERATED true)

    else(${rcgen_result} EQUAL 0)

      message(STATUS "Failed to generate Nepomuk resource classes.")

    endif(${rcgen_result} EQUAL 0)

  endif(NOT RCGEN)

endmacro(NEPOMUK_GENERATE_FROM_ONTOLOGY2)


NEPOMUK_GENERATE_FROM_ONTOLOGY2(
  ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
  ${CMAKE_CURRENT_BINARY_DIR}
  NCO_HEADERS
  NCO_SOURCES
  DUMMY
  )

if(NEPOMUK_RESOURCES_GENERATED)

  set(HAVE_NIE 1)

  kde4_add_library(nie STATIC ${NCO_SOURCES} ${NIE_SOURCES})

  target_link_libraries(nie
    ${QT_QTCORE_LIBRARY}
    ${NEPOMUK_LIBRARIES}
    )

endif(NEPOMUK_RESOURCES_GENERATED)
