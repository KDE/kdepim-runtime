include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${NEPOMUK_INCLUDES}
  dms-copy
  plugin
  ./
  ${CMAKE_CURRENT_BINARY_DIR}
  )

include( SopranoAddOntology )

add_subdirectory( dms-copy )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -fPIC" )

add_subdirectory( plugin )
add_subdirectory( plugins )
add_subdirectory( test )

set(CMAKE_NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL 3)
configure_file(nepomukfeeder-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/nepomukfeeder-config.h)

kde4_add_executable(akonadi_nepomuk_feeder nepomukfeederagent.cpp pluginloader.cpp feederqueue.cpp nepomukhelpers.cpp)

set( NEPOMUKFEEDER_SHARED_LIBS
    niefast
    nepomukdatamanagement-copy
    ${SOPRANO_LIBRARIES}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${NEPOMUK_LIBRARIES}
    ${STRIGI_STREAMANALYZER_LIBRARY}
    ${STRIGI_STREAMS_LIBRARY}
    ${KDE4_KIDLETIME_LIBRARY}
)

target_link_libraries(akonadi_nepomuk_feeder
  ${NEPOMUKFEEDER_SHARED_LIBS}
  nepomukfeederpluginlib
)



install(TARGETS akonadi_nepomuk_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES akonadinepomukfeederagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(
   FILES akonadinepomukfeeder.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR})

