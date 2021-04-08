
include_directories(${kdepim-runtime_BINARY_DIR})
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_newmailnotifier_agent\")


set(newmailnotifier_common_SRCS)

kconfig_add_kcfg_files(newmailnotifier_common_SRCS
    newmailnotifieragentsettings.kcfgc
  )

ecm_qt_declare_logging_category(newmailnotifier_common_SRCS HEADER newmailnotifier_debug.h IDENTIFIER NEWMAILNOTIFIER_LOG CATEGORY_NAME org.kde.pim.newmailnotifier
        DESCRIPTION "mailnotifier agent (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_newmailnotifier
        EXPORT KDEPIMRUNTIME
    )


set(newmailnotifieragent_SRCS
  newmailnotifieragent.cpp
  specialnotifierjob.cpp
  newmailnotifiershowmessagejob.cpp
  ${newmailnotifier_common_SRCS}
)

qt5_add_dbus_adaptor(newmailnotifieragent_SRCS org.freedesktop.Akonadi.NewMailNotifier.xml newmailnotifieragent.h NewMailNotifierAgent)

add_executable( akonadi_newmailnotifier_agent ${newmailnotifieragent_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_newmailnotifier_agent PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries( akonadi_newmailnotifier_agent
  KF5::AkonadiCore
  KF5::Mime
  KF5::AkonadiMime
  KF5::AkonadiContact
  KF5::Codecs
  KF5::IdentityManagement
  KF5::AkonadiAgentBase
  KF5::Notifications
  KF5::Service
  KF5::I18n
  Qt::TextToSpeech
)

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

########################## Config plugin #################################

kcoreaddons_add_plugin(newmailnotifierconfig
    SOURCES newmailnotifiersettingswidget.cpp
            newmailnotifierselectcollectionwidget.cpp
            ${newmailnotifier_common_SRCS}
    JSON "newmailnotifierconfig.json"
    INSTALL_NAMESPACE "akonadi/config")
target_link_libraries(newmailnotifierconfig
    KF5::AkonadiCore
    KF5::AkonadiMime
    Qt::Widgets
    KF5::NotifyConfig
    KF5::ConfigGui
    KF5::I18n
    KF5::XmlGui
    KF5::Mime
    KF5::Completion
)
