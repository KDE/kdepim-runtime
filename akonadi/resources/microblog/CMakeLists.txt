include_directories( ${Boost_INCLUDE_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( microblogresource_SRCS
  microblog.cpp
  configdialog.cpp
  settings.cpp
  communication.cpp
  idattribute.cpp
)

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/microblogresource.kcfg org.kde.Akonadi.Microblog.Settings)
qt4_add_dbus_adaptor( microblogresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Microblog.Settings.xml settings.h Settings
)

kde4_add_ui_files(microblogresource_SRCS microblogconfig.ui)
kde4_add_kcfg_files(microblogresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_microblog_resource RUN_UNINSTALLED ${microblogresource_SRCS})

target_link_libraries(akonadi_microblog_resource ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY} ${KDEPIMLIBS_KPIMUTILS_LIBS} ${KDEPIMLIBS_MICROBLOG_LIBS})

install(TARGETS akonadi_microblog_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES microblog.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
