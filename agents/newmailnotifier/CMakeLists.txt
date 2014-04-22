kde_enable_exceptions()
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)
#find_package(KF5AkonadiContact ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiKMime ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KMime ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig CONFIG REQUIRED)

set(newmailnotifieragent_SRCS
  newmailnotifiersettingsdialog.cpp
  newmailnotifieragent.cpp
  newmailnotifierattribute.cpp
  specialnotifierjob.cpp
  newmailnotifierselectcollectionwidget.cpp
  util.cpp
)

kconfig_add_kcfg_files(newmailnotifieragent_SRCS
    newmailnotifieragentsettings.kcfgc
  )


qt5_add_dbus_adaptor(newmailnotifieragent_SRCS org.freedesktop.Akonadi.NewMailNotifier.xml newmailnotifieragent.h NewMailNotifierAgent)


add_executable( akonadi_newmailnotifier_agent ${newmailnotifieragent_SRCS})


target_link_libraries( akonadi_newmailnotifier_agent
  KF5::AkonadiCore
  KF5::KMime
  KF5::AkonadiKMime
  KF5::AkonadiContact
  KF5::PimUtils
  KF5::PimIdentities
  KF5::NotifyConfig
  KF5::AkonadiAgentBase
  
)


if (Q_WS_MAC)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.newmailnotifier")
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE New Mail Notifier")
endif ()

install(TARGETS akonadi_newmailnotifier_agent ${INSTALL_TARGETS_DEFAULT_ARGS} )


install(FILES newmailnotifieragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(FILES akonadi_newmailnotifier_agent.notifyrc DESTINATION "${DATA_INSTALL_DIR}/akonadi_newmailnotifier_agent" )

add_subdirectory(kconf_update)
