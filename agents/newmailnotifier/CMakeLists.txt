
include_directories(${kdepim-runtime_BINARY_DIR})
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_newmailnotifier_agent\")

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

set(newmailnotifieragent_SRCS
  newmailnotifier_debug.cpp
  newmailnotifiersettingsdialog.cpp
  newmailnotifieragent.cpp
  specialnotifierjob.cpp
  newmailnotifierselectcollectionwidget.cpp
  newmailnotifiershowmessagejob.cpp
)

kconfig_add_kcfg_files(newmailnotifieragent_SRCS
    newmailnotifieragentsettings.kcfgc
  )


qt5_add_dbus_adaptor(newmailnotifieragent_SRCS org.freedesktop.Akonadi.NewMailNotifier.xml newmailnotifieragent.h NewMailNotifierAgent)


add_executable( akonadi_newmailnotifier_agent ${newmailnotifieragent_SRCS})


target_link_libraries( akonadi_newmailnotifier_agent
  KF5::AkonadiCore
  KF5::Mime
  KF5::AkonadiMime
  KF5::AkonadiContact
  KF5::Codecs
  KF5::IdentityManagement
  KF5::NotifyConfig
  KF5::AkonadiAgentBase
)


if (Q_WS_MAC)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.newmailnotifier")
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE New Mail Notifier")
endif ()

install(TARGETS akonadi_newmailnotifier_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


install(FILES newmailnotifieragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(FILES akonadi_newmailnotifier_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )

add_subdirectory(kconf_update)
