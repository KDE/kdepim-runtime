set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(calendarresource_SRCS
  calendareditor.cpp
  calendarresource.cpp
  defaultreminderattribute.cpp
  resource_tasks.cpp
  resource_events.cpp
  settings.cpp
  settingsdialog.cpp
  tasklisteditor.cpp
)

kde4_add_ui_files(calendarresource_SRCS
  ui/settingsdialog.ui
  ui/calendar_editor.ui
  ui/tasklist_editor.ui
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

target_link_libraries(akonadi_googlecalendar_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KCAL_LIBS}
  ${KDEPIMLIBS_KCALCORE_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEWEBKIT_LIBS}
  ${QJSON_LIBRARIES}
  ${LIBKGOOGLE_LIBRARY}
)

install(TARGETS akonadi_googlecalendar_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googlecalendarresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
