set( mtdummyresource_SRCS
  configdialog.cpp
  mtdummyresource.cpp
)

# mailtransport debug area
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5324)

install( FILES mtdummyresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(mtdummyresource_SRCS settings.ui)
kde4_add_kcfg_files(mtdummyresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mtdummyresource.kcfg org.kde.Akonadi.MailTransportDummy.Settings)
qt4_add_dbus_adaptor(mtdummyresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailTransportDummy.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_mailtransport_dummy_resource RUN_UNINSTALLED ${mtdummyresource_SRCS})

target_link_libraries(akonadi_mailtransport_dummy_resource ${KDE4_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS})

install(TARGETS akonadi_mailtransport_dummy_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
