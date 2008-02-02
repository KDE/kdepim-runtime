macro(NEPOMUK_GENERATE_FROM_ONTOLOGY2 ontofile1 ontofile2 targetdir out_headers out_sources out_includes)

  FIND_PROGRAM(RCGEN nepomuk-rcgen PATHS ${BIN_INSTALL_DIR})
  if(NOT RCGEN)
    message( FATAL_ERROR "Failed to find the KMetaData source generator" )
  endif(NOT RCGEN)

  FILE(TO_NATIVE_PATH ${RCGEN} RCGEN)

  execute_process(
    COMMAND ${RCGEN} --quiet --listheaders --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
    OUTPUT_VARIABLE ${out_headers}
    )
  
  execute_process(
    COMMAND ${RCGEN} --quiet --listsources --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
    OUTPUT_VARIABLE ${out_sources}
    )
  
  execute_process(
    COMMAND ${RCGEN} --quiet --listincludes --ontologies ${ontofile1} ${ontofile2}
    OUTPUT_VARIABLE ${out_includes}
    )

  execute_process(
    COMMAND ${RCGEN} --quiet --writeall --templates ${ARGN} --target ${targetdir}/ --ontologies ${ontofile1} ${ontofile2}
    )
  
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

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(akonadi_nepomuk_contact_feeder RUN_UNINSTALLED nepomukcontactfeeder.cpp ${NCO_SOURCES} ${NIE_SOURCES})

target_link_libraries(akonadi_nepomuk_contact_feeder
  akonadi
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KABC_LIBS}
  ${NEPOMUK_LIBRARY}
)

install(TARGETS akonadi_nepomuk_contact_feeder DESTINATION ${BIN_INSTALL_DIR})
install(FILES nepomukcontactfeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
