kde_enable_exceptions()

include_directories(${QJSON_INCLUDE_DIR}
                    ${qjson_INCLUDE_DIR}
)

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

kconfig_add_kcfg_files(calendarresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.GoogleCalendar.Settings
)

qt5_add_dbus_adaptor(calendarresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.GoogleCalendar.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)

add_executable(akonadi_googlecalendar_resource RUN_UNINSTALLED ${calendarresource_SRCS})

if(Q_WS_MAC)
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
  ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}
  KF5::CalendarCore
  KF5::Mime
  
  
  
  
  
  ${KDE4_KDEWEBKIT_LIBS}
  ${QJSON_LIBRARIES}
  ${QJSON_LIBRARY} # for Mac OSX
  ${qjson_LIBRARY} # for Debian
  ${LibKGAPI2_LIBRARY}
)

if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
  target_link_libraries(akonadi_googlecalendar_resource
    ${ACCOUNTSQT_LIBRARIES}
    ${SIGNONQT_LIBRARIES})
endif()


install(TARGETS akonadi_googlecalendar_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googlecalendarresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
