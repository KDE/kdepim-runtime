include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

if ( STRIGIQTDBUSCLIENT_LIBRARY )

include_directories(${STRIGI_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR}/strigi)

set(strigi_feeder_SRCS
  configdialog.cpp
  strigifeeder.cpp
)

kde4_add_kcfg_files(strigi_feeder_SRCS settings.kcfgc)
kde4_add_ui_files(strigi_feeder_SRCS configdialog.ui)

if (RUNTIME_PLUGINS_STATIC)
  add_definitions(-DSERIALIZER_PLUGINS_STATIC)
endif (RUNTIME_PLUGINS_STATIC)

kde4_add_executable(akonadi_strigi_feeder ${strigi_feeder_SRCS})
target_link_libraries(akonadi_strigi_feeder
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${STRIGIQTDBUSCLIENT_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${KDE4_KIDLETIME_LIBRARY}
)

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_strigi_feeder akonadi_serializer_mail akonadi_serializer_addressee akonadi_serializer_contactgroup akonadi_serializer_kcalcore)
endif (RUNTIME_PLUGINS_STATIC)

install(TARGETS akonadi_strigi_feeder ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES strigifeeder.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

endif ( STRIGIQTDBUSCLIENT_LIBRARY )
