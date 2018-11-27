
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_googlecalendar_resource\")


set(calendarresource_SRCS
  calendarresource.cpp
  defaultreminderattribute.cpp
  settings.cpp
  settingsdialog.cpp
  ../common/googleresource.cpp
  ../common/googleaccountmanager.cpp
  ../common/googlesettings.cpp
  ../common/googlesettingsdialog.cpp
  ${accounts_SRCS}
)
ecm_qt_declare_logging_category(calendarresource_SRCS HEADER googlecalendarresource_debug.h IDENTIFIER GOOGLE_CALENDAR_LOG CATEGORY_NAME org.kde.pim.google.calendar)
kconfig_add_kcfg_files(calendarresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.GoogleCalendar.Settings
)

qt5_add_dbus_adaptor(calendarresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.GoogleCalendar.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)

add_executable(akonadi_googlecalendar_resource ${calendarresource_SRCS})

if( APPLE )
  set_target_properties(akonadi_googlecalendar_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
  )
  set_target_properties(akonadi_googlecalendar_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.googlecalendar"
  )
  set_target_properties(akonadi_googlecalendar_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Google Calendar Resource"
  )
endif()




target_link_libraries(akonadi_googlecalendar_resource
  KF5::AkonadiCore
  KF5::CalendarCore
  KF5::AkonadiCalendar
  KPim::GAPICalendar
  KPim::GAPICore
  KPim::GAPITasks
  KF5::AkonadiAgentBase
  KF5::Wallet
  KF5::I18n
  KF5::WindowSystem
  KF5::Completion
  KF5::TextWidgets
)

install(TARGETS akonadi_googlecalendar_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googlecalendarresource.desktop
  DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents"
)
