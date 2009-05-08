include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( icalresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  icalresource.cpp
)

install( FILES icalresource.desktop notesresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(icalresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(icalresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icalresource.kcfg org.kde.Akonadi.ICal.Settings)
qt4_add_dbus_adaptor(icalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICal.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_ical_resource RUN_UNINSTALLED ${icalresource_SRCS})

target_link_libraries(akonadi_ical_resource akonadi-kcal ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${KDEPIMLIBS_KCAL_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS akonadi_ical_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

