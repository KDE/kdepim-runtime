add_subdirectory( maildispatcher )

if( Nepomuk_FOUND AND STRIGI_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND )
    include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/nepomukfeeder ${CMAKE_CURRENT_BINARY_DIR}/ontologies ${STRIGI_INCLUDE_DIR})
    set( NEPOMUKFEEDER_SHARED_SRCS
         ${CMAKE_CURRENT_SOURCE_DIR}/nepomukfeeder/nepomukfeederagentbase.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/nepomukfeeder/sparqlbuilder.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/nepomukfeeder/selectsparqlbuilder.cpp
    )
    set( NEPOMUKFEEDER_SHARED_LIBS
      niefast
      ${SOPRANO_LIBRARIES}
      ${KDE4_KDEUI_LIBRARY}
      ${KDE4_KDECORE_LIBS}
      ${KDEPIMLIBS_AKONADI_LIBS}
      ${QT_QTCORE_LIBRARY}
      ${QT_QTDBUS_LIBRARY}
      ${NEPOMUK_LIBRARIES}
      ${STRIGI_STREAMANALYZER_LIBRARY}
      ${STRIGI_STREAMS_LIBRARY}
    )
    include( SopranoAddOntology )
    add_subdirectory( ontologies )
    add_subdirectory( nepomukfeeder )
    add_subdirectory( nepomuk_contact_feeder )
    add_subdirectory( nepomuk_calendar_feeder )
endif( Nepomuk_FOUND AND STRIGI_FOUND AND SOPRANO_PLUGIN_RAPTORPARSER_FOUND )
