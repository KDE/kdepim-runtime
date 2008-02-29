
include(NepomukMacros)

# FIXME: Make this macro take a list of ontology files and then move it to NepomukMacros.cmake
macro(NEPOMUK_GENERATE_FROM_ONTOLOGY2 ontofile1 ontofile2 targetdir out_headers out_sources out_includes)

  # init
  set(NEPOMUK_RESOURCES_GENERATED false)

  FIND_PROGRAM(RCGEN nepomuk-rcgen PATHS ${BIN_INSTALL_DIR})

  if(NOT RCGEN)

    message(STATUS "Failed to find the Nepomuk source generator" )

  else(NOT RCGEN)

    FILE(TO_NATIVE_PATH ${RCGEN} RCGEN)

    execute_process(
      COMMAND ${RCGEN} --listheaders --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
      OUTPUT_VARIABLE ${out_headers}
      RESULT_VARIABLE rcgen_result
      )

    # If the first call succeeds it is very very likely that the rest will, too
    if(${rcgen_result} EQUAL 0)

      execute_process(
        COMMAND ${RCGEN} --listsources --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
        OUTPUT_VARIABLE ${out_sources}
        )
      
      execute_process(
        COMMAND ${RCGEN} --listincludes --ontologies ${ontofile1} ${ontofile2}
        OUTPUT_VARIABLE ${out_includes}
        )

      execute_process(
        COMMAND ${RCGEN} --writeall --templates ${ARGN} --target ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
        )

      set(NEPOMUK_RESOURCES_GENERATED true)

    else(${rcgen_result} EQUAL 0)

      message(STATUS "Failed to generate Nepomuk resource classes.")

    endif(${rcgen_result} EQUAL 0)

  endif(NOT RCGEN)
  
endmacro(NEPOMUK_GENERATE_FROM_ONTOLOGY2)

include_directories(
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_SOURCE_DIR}/akonadi/libakonadi
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${NEPOMUK_INCLUDES}
  )

NEPOMUK_GENERATE_FROM_ONTOLOGY2(
  ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
  ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
  ${CMAKE_CURRENT_BINARY_DIR}
  NCO_HEADERS
  NCO_SOURCES
  DUMMY
  )

if(NEPOMUK_RESOURCES_GENERATED)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

  kde4_add_executable(akonadi_nepomuk_contact_feeder RUN_UNINSTALLED nepomukcontactfeeder.cpp ${NCO_SOURCES} ${NIE_SOURCES})

  target_link_libraries(akonadi_nepomuk_contact_feeder
    akonadicore
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KABC_LIBS}
    ${NEPOMUK_LIBRARY}
    )

  install(TARGETS akonadi_nepomuk_contact_feeder DESTINATION ${BIN_INSTALL_DIR})
  install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif(NEPOMUK_RESOURCES_GENERATED)
