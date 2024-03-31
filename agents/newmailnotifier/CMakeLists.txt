
include_directories(${kdepim-runtime_BINARY_DIR})
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_newmailnotifier_agent\")
configure_file(config-newmailnotifier.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-newmailnotifier.h)


add_library(newmailnotifier)

target_sources(newmailnotifier PRIVATE
    newmailnotificationhistorymanager.h
    newmailnotificationhistorymanager.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(newmailnotifier PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(newmailnotifier BASE_NAME newmailnotifier)

target_link_libraries(newmailnotifier
    PRIVATE
    KPim6::AkonadiCore
    KF6::I18n
)

set_target_properties(newmailnotifier PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS newmailnotifier ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)


add_executable( akonadi_newmailnotifier_agent)
set(newmailnotifier_common_SRCS)
kconfig_add_kcfg_files(newmailnotifier_common_SRCS
    newmailnotifieragentsettings.kcfgc
  )

ecm_qt_declare_logging_category(newmailnotifier_common_SRCS HEADER newmailnotifier_debug.h IDENTIFIER NEWMAILNOTIFIER_LOG CATEGORY_NAME org.kde.pim.newmailnotifier
        DESCRIPTION "mailnotifier agent (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_newmailnotifier
        EXPORT KDEPIMRUNTIME
    )



qt_add_dbus_adaptor(newmailnotifieragent_SRCS org.freedesktop.Akonadi.NewMailNotifier.xml newmailnotifieragent.h NewMailNotifierAgent)
target_sources(akonadi_newmailnotifier_agent PRIVATE
  newmailnotifieragent.cpp
  newmailnotifieragent.h
  specialnotifierjob.cpp
  specialnotifierjob.h
  newmailnotifiershowmessagejob.cpp
  newmailnotifiershowmessagejob.h
  newmailnotifierreplymessagejob.cpp
  newmailnotifierreplymessagejob.h

  newmailnotifieropenfolderjob.h
  newmailnotifieropenfolderjob.cpp

  newmailnotificationhistorywidget.h
  newmailnotificationhistorywidget.cpp
  newmailnotificationhistorydialog.h
  newmailnotificationhistorydialog.cpp
  newmailnotificationhistorybrowsertextwidget.h
  newmailnotificationhistorybrowsertextwidget.cpp

  newmailnotificationhistorybrowsertext.h
  newmailnotificationhistorybrowsertext.cpp

  ${newmailnotifier_common_SRCS}
  ${newmailnotifieragent_SRCS}
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_newmailnotifier_agent PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries( akonadi_newmailnotifier_agent
    PRIVATE
  KPim6::AkonadiCore
  KPim6::Mime
  KPim6::AkonadiMime
  KPim6::AkonadiContactWidgets
  KF6::Codecs
  KPim6::IdentityManagementCore
  KPim6::AkonadiAgentBase
  KF6::Notifications
  KF6::Service
  KF6::I18n
  KF6::KIOGui
  KF6::WindowSystem
  KF6::ConfigWidgets
  KF6::TextCustomEditor
  newmailnotifier
)

if(Qt6TextToSpeech_FOUND)
    target_link_libraries( akonadi_newmailnotifier_agent PRIVATE
      Qt::TextToSpeech
    )
endif()

if( APPLE )
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.template)
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.newmailnotifier")
  set_target_properties( akonadi_newmailnotifier_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE New Mail Notifier")
endif ()

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_subdirectory(autotests)
endif()
install(TARGETS akonadi_newmailnotifier_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )


install(FILES newmailnotifieragent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES akonadi_newmailnotifier_agent.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR} )
akonadi_install_systemd_unit(AGENT TARGET akonadi_newmailnotifier_agent DESCRIPTION "Akonadi New Mail Notifier Agent")

########################## Config plugin #################################

kcoreaddons_add_plugin(newmailnotifierconfig
    INSTALL_NAMESPACE "pim6/akonadi/config")

target_sources(newmailnotifierconfig PRIVATE
    newmailnotifiersettingswidget.cpp
    newmailnotifierselectcollectionwidget.cpp
    newmailnotifiersettingswidget.h
    newmailnotifierselectcollectionwidget.h
    ${newmailnotifier_common_SRCS}
    )

target_link_libraries(newmailnotifierconfig
    PRIVATE
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    Qt::Widgets
    KF6::NotifyConfig
    KF6::ConfigGui
    KF6::I18n
    KF6::XmlGui
    KPim6::Mime
    KF6::Completion
)

