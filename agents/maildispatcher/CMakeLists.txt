include_directories(
    ${AKONADI_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( maildispatcheragent_SRCS
  maildispatcheragent.cpp
  maildispatcherattribute.cpp
  configdialog.cpp
)

add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5266 )

kde4_add_ui_files(maildispatcheragent_SRCS settings.ui)
kde4_add_kcfg_files(maildispatcheragent_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildispatcheragent.kcfg org.kde.Akonadi.MailDispatcher.Settings)
qt4_add_dbus_adaptor(maildispatcheragent_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailDispatcher.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_maildispatcher_agent RUN_UNINSTALLED ${maildispatcheragent_SRCS})

target_link_libraries(akonadi_maildispatcher_agent ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KMIME_LIBS} )

install(TARGETS akonadi_maildispatcher_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES maildispatcheragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
