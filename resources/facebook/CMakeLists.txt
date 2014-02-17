project(facebookresource)
include_directories(${LibKFbAPI_INCLUDE_DIR})
include_directories(${QJSON_INCLUDE_DIR} ${qjson_INCLUDE_DIR})
if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
  include_directories(${ACCOUNTSQT_INCLUDE_DIRS} ${SIGNONQT_INCLUDE_DIRS})
  add_definitions(-DHAVE_ACCOUNTS)
  set(facebookresource_SRCS ../shared/getcredentialsjob.cpp)
endif()

set(facebookresource_SRCS
  facebookresource.cpp
  facebookresource_events.cpp
  facebookresource_friends.cpp
  facebookresource_notes.cpp
  facebookresource_posts.cpp
  facebookresource_notifications.cpp
  settings.cpp
  settingsdialog.cpp
  timestampattribute.cpp
  ${facebookresource_SRCS}
)

install(FILES facebookresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(FILES akonadi_facebook_resource.notifyrc DESTINATION "${DATA_INSTALL_DIR}/akonadi_facebook_resource")

kde4_add_ui_files(facebookresource_SRCS settingsdialog.ui)
kde4_add_kcfg_files(facebookresource_SRCS settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Facebook.Settings
)

qt4_add_dbus_adaptor(facebookresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Facebook.Settings.xml
  settings.h
  Settings
)

kde4_add_executable(akonadi_facebook_resource RUN_UNINSTALLED ${facebookresource_SRCS})

if(Q_WS_MAC)
  set_target_properties(
    akonadi_facebook_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template
  )
  set_target_properties(
    akonadi_facebook_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Facebook"
  )
  set_target_properties(
    akonadi_facebook_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Facebook Resource"
  )
endif()

include_directories(${Boost_INCLUDE_DIR})

target_link_libraries(akonadi_facebook_resource
  ${KDEPIMLIBS_AKONADI_SOCIALUTILS_LIBS}
  ${KDEPIMLIBS_AKONADI_NOTES_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${KDEPIMLIBS_KCALCORE_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QJSON_LIBRARIES}
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${LibKFbAPI_LIBRARY}
)

if(${AccountsQt_FOUND} AND ${SignOnQt_FOUND})
  target_link_libraries(akonadi_facebook_resource
    ${ACCOUNTSQT_LIBRARIES}
    ${SIGNONQT_LIBRARIES})
endif()

add_subdirectory(icons)
add_subdirectory(serializer)

install(TARGETS akonadi_facebook_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
