add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_google_resource\")

set(googleresource_common_SRCS
    googlesettings.cpp
    googlesettings.h
    googlescopes.h
    googlescopes.cpp
)

ecm_qt_declare_logging_category(googleresource_common_SRCS
    HEADER googleresource_debug.h
    IDENTIFIER GOOGLE_LOG
    CATEGORY_NAME org.kde.pim.google
    DESCRIPTION "resource google (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_common_SRCS
    HEADER googlecalendar_debug.h
    IDENTIFIER GOOGLE_CALENDAR_LOG
    CATEGORY_NAME org.kde.pim.google.calendar
    DESCRIPTION "resource google calendar (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_common_SRCS
    HEADER googletasks_debug.h
    IDENTIFIER GOOGLE_TASKS_LOG
    CATEGORY_NAME org.kde.pim.google.tasks
    DESCRIPTION "resource google tasks (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_common_SRCS
    HEADER googlepeople_debug.h
    IDENTIFIER GOOGLE_PEOPLE_LOG
    CATEGORY_NAME org.kde.pim.google.people
    DESCRIPTION "resource google people (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)


kconfig_add_kcfg_files(googleresource_common_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Google.Settings
)

qt_add_dbus_adaptor(googleresource_common_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Google.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/googlesettings.h GoogleSettings
)

############################# Resource #################################
set(googleresource_SRCS
    googleresource.cpp
    defaultreminderattribute.cpp
    googleresourcestate.cpp
    generichandler.cpp
    calendarhandler.cpp
    taskhandler.cpp
    peopleconversionjob.cpp
    personhandler.cpp
    googleresource.h
    googlesettings.h
    defaultreminderattribute.h
    googleresourcestate.h
    generichandler.h
    calendarhandler.h
    taskhandler.h
    peopleconversionjob.h
    personhandler.h
    ${googleresource_common_SRCS}
)

ki18n_wrap_ui(googleresource_SRCS googlesettingswidget.ui)


add_executable(akonadi_google_resource ${googleresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_google_resource PROPERTIES UNITY_BUILD ON)
endif()

if( APPLE )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
  )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.google"
  )
  set_target_properties(akonadi_google_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Google Resource"
  )
endif()

target_link_libraries(akonadi_google_resource
  KPim6::AkonadiCalendar
  KPim6::AkonadiCore
  KPim6::AkonadiAgentBase
  KF6::CalendarCore
  KF6::Contacts
  KF6::I18n
  KF6::WindowSystem
  KF6::TextWidgets
  KF6::Notifications
  KPim6::GAPICalendar
  KPim6::GAPIPeople
  KPim6::GAPICore
  KPim6::GAPITasks
  qt6keychain
)

install(TARGETS akonadi_google_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googleresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)

install(
    FILES org.kde.akonadi_google_resource.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)

install(
  FILES akonadi_google_resource.notifyrc
  DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

################################ Config plugin ###############################

kcoreaddons_add_plugin(googleconfig
    INSTALL_NAMESPACE "pim${QT_MAJOR_VERSION}/akonadi/config"
)
target_sources(googleconfig
PRIVATE
    googleconfig.cpp
    googlesettingswidget.cpp
    googlesettingswidget.h
    ${googleresource_common_SRCS}
)
target_link_libraries(googleconfig
    KPim6::AkonadiCore
    KPim6::AkonadiCalendar
    KPim6::AkonadiCore
    KPim6::AkonadiAgentBase
    KF6::CalendarCore
    KF6::Contacts
    KF6::Wallet
    KF6::I18n
    KF6::WindowSystem
    KF6::TextWidgets
    KF6::Notifications
    KPim6::GAPICalendar
    KPim6::GAPIPeople
    KPim6::GAPICore
    KPim6::GAPITasks
    qt6keychain
)
