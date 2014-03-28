set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

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

kde4_add_kcfg_files(calendarresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.GoogleCalendar.Settings
)

qt4_add_dbus_adaptor(calendarresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.GoogleCalendar.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)

kde4_add_executable(akonadi_googlecalendar_resource RUN_UNINSTALLED ${calendarresource_SRCS})

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
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_CALENDAR_LIBS}
  ${KDEPIMLIBS_KCALCORE_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
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
