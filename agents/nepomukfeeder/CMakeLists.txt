include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${NEPOMUK_CORE_INCLUDE_DIR}
  plugin
  ./
  ${CMAKE_CURRENT_BINARY_DIR}
  )

include( SopranoAddOntology )

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(malloc_trim  HAVE_MALLOC_TRIM)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -fPIC" )

add_subdirectory( plugin )
add_subdirectory( plugins )
add_subdirectory( test )
add_subdirectory( util )

set(CMAKE_NEPOMUK_FEEDER_INDEX_COMPAT_LEVEL 3)
configure_file(nepomukfeeder-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/nepomukfeeder-config.h)

set(akonadi_nepomuk_feeder_SRCS nepomukfeederagent.cpp pluginloader.cpp feederqueue.cpp itemqueue.cpp nepomukhelpers.cpp propertycache.cpp findunindexeditemsjob.cpp nepomukcleanerjob.cpp nepomukfeederagentdialog.cpp)

qt4_add_dbus_adaptor(akonadi_nepomuk_feeder_SRCS org.freedesktop.Akonadi.NepomukFeeder.xml nepomukfeederagent.h Akonadi::NepomukFeederAgent)

kde4_add_executable(akonadi_nepomuk_feeder ${akonadi_nepomuk_feeder_SRCS})


set( NEPOMUKFEEDER_SHARED_LIBS
    niefast
    ${SOPRANO_LIBRARIES}
    ${KDE4_KDEUI_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${NEPOMUK_CORE_LIBRARY}
    ${STRIGI_STREAMANALYZER_LIBRARY}
    ${STRIGI_STREAMS_LIBRARY}
    ${KDE4_KIDLETIME_LIBRARY}
)

if (Q_WS_MAC)
  set_target_properties(akonadi_nepomuk_feeder PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_nepomuk_feeder PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.nepomukfeeder")
  set_target_properties(akonadi_nepomuk_feeder PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Nepomukfeeder")
endif (Q_WS_MAC)


target_link_libraries(akonadi_nepomuk_feeder
  ${NEPOMUKFEEDER_SHARED_LIBS}
  nepomukfeederpluginlib
)



install(TARGETS akonadi_nepomuk_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES akonadi_nepomuk_feeder.notifyrc DESTINATION "${DATA_INSTALL_DIR}/akonadi_nepomuk_feeder" )
install(FILES akonadinepomukfeederagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(
   FILES akonadinepomukfeeder.desktop
   DESTINATION ${SERVICETYPES_INSTALL_DIR})

