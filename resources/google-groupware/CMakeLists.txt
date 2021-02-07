add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_google_resource\")

set(googleresource_SRCS
    googleresource.cpp
    googlesettings.cpp
    googlesettingsdialog.cpp
    defaultreminderattribute.cpp
    googleresourcestate.cpp
    generichandler.cpp
    calendarhandler.cpp
    taskhandler.cpp
    contacthandler.cpp)

ecm_qt_declare_logging_category(googleresource_SRCS
    HEADER googleresource_debug.h
    IDENTIFIER GOOGLE_LOG
    CATEGORY_NAME org.kde.pim.google
    DESCRIPTION "resource google (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_SRCS
    HEADER googlecalendar_debug.h
    IDENTIFIER GOOGLE_CALENDAR_LOG
    CATEGORY_NAME org.kde.pim.google.calendar
    DESCRIPTION "resource google calendar (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_SRCS
    HEADER googletasks_debug.h
    IDENTIFIER GOOGLE_TASKS_LOG
    CATEGORY_NAME org.kde.pim.google.tasks
    DESCRIPTION "resource google tasks (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)
ecm_qt_declare_logging_category(googleresource_SRCS
    HEADER googlecontacts_debug.h
    IDENTIFIER GOOGLE_CONTACTS_LOG
    CATEGORY_NAME org.kde.pim.google.contacts
    DESCRIPTION "resource google contacts (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME)

ki18n_wrap_ui(googleresource_SRCS googlesettingsdialog.ui)

kconfig_add_kcfg_files(googleresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Google.Settings
)

qt5_add_dbus_adaptor(googleresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Google.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/googlesettings.h GoogleSettings
)

add_executable(akonadi_google_resource ${googleresource_SRCS})

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
  KF5::AkonadiCalendar
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::CalendarCore
  KF5::Contacts
  KF5::Wallet
  KF5::I18n
  KF5::WindowSystem
  KF5::TextWidgets
  KF5::Notifications
  KPim::GAPICalendar
  KPim::GAPIContacts
  KPim::GAPICore
  KPim::GAPITasks
)

install(TARGETS akonadi_google_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googleresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)

install(
  FILES akonadi_google_resource.notifyrc
  DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR}
)
