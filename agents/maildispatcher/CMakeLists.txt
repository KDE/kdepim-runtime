
add_subdirectory( autotests )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildispatcher_agent\")

set( maildispatcheragent_SRCS
  maildispatcher_debug.cpp
  maildispatcheragent.cpp
  outboxqueue.cpp
  sendjob.cpp
  sentactionhandler.cpp
  storeresultjob.cpp
  maildispatcherresourcefactory.cpp
)

ki18n_wrap_ui(maildispatcheragent_SRCS settings.ui)
kconfig_add_kcfg_files(maildispatcheragent_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildispatcheragent.kcfg org.kde.Akonadi.MailDispatcher.Settings)
qt5_add_dbus_adaptor(maildispatcheragent_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailDispatcher.Settings.xml settings.h Settings
)
qt5_add_dbus_adaptor( maildispatcheragent_SRCS
  org.freedesktop.Akonadi.MailDispatcherAgent.xml maildispatcheragent.h MailDispatcherAgent
)

if (RUNTIME_PLUGINS_STATIC)
  add_definitions(-DMAIL_SERIALIZER_PLUGIN_STATIC)
endif ()

kdepim_add_agent(akonadi_maildispatcher_agent ${maildispatcheragent_SRCS})

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_maildispatcher_agent akonadi_serializer_mail)
endif ()

if (Q_WS_MAC)
  set_target_properties(akonadi_maildispatcher_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_maildispatcher_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.maildispatcher")
  set_target_properties(akonadi_maildispatcher_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Maildispatcher")
endif ()

target_link_libraries(akonadi_maildispatcher_agent KF5::AkonadiCore KF5::AkonadiMime KF5::KIOCore KF5::Mime KF5::MailTransport KF5::AkonadiAgentBase KF5::NotifyConfig)

install( FILES maildispatcheragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
install( FILES akonadi_maildispatcher_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
