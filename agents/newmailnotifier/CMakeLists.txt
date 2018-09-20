
include_directories(${kdepim-runtime_BINARY_DIR})
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_newmailnotifier_agent\")

set(newmailnotifieragent_SRCS
  newmailnotifiersettingsdialog.cpp
  newmailnotifieragent.cpp
  specialnotifierjob.cpp
  newmailnotifierselectcollectionwidget.cpp
  newmailnotifiershowmessagejob.cpp
)

ecm_qt_declare_logging_category(newmailnotifieragent_SRCS HEADER newmailnotifier_debug.h IDENTIFIER NEWMAILNOTIFIER_LOG CATEGORY_NAME org.kde.pim.newmailnotifier)

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
  KF5::DBusAddons
  KF5::XmlGui
  KF5::Notifications
  KF5::WindowSystem
  KF5::Completion
  KF5::Service
  KF5::IconThemes
  KF5::I18n
)

if (Qt5TextToSpeech_FOUND)
    target_link_libraries(akonadi_newmailnotifier_agent
        Qt5::TextToSpeech)
endif()

if( APPLE )
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.newmailnotifier")
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE New Mail Notifier")
endif ()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
install(TARGETS akonadi_newmailnotifier_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


install(FILES newmailnotifieragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_newmailnotifier_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )

