project(facebookresource)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_facebook_resource\")

include_directories(${ACCOUNTSQT_INCLUDE_DIRS} ${SIGNONQT_INCLUDE_DIRS})

set(facebookresource_SRCS
  facebookresource.cpp
  facebookresource_events.cpp
  facebookresource_notes.cpp
  facebookresource_posts.cpp
  facebookresource_notifications.cpp
  settings.cpp
#   settingsdialog.cpp
  timestampattribute.cpp
  ${facebookresource_SRCS}
)

ki18n_wrap_ui(facebookresource_SRCS settingsdialog.ui)
kconfig_add_kcfg_files(facebookresource_SRCS settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.Facebook.Settings
)

qt5_add_dbus_adaptor(facebookresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Facebook.Settings.xml
  settings.h
  Settings
)

add_executable(akonadi_facebook_resource ${facebookresource_SRCS})

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
  Qt5::Core
  Qt5::Gui
  KF5::I18n
  KF5::Notifications
  KF5::Syndication
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::Contacts
  KF5::CalendarCore
  KF5::AkonadiNotes
  KF5::Mime
  KF5::PimUtils
  KF5::FbAPI
  KF5::AkonadiSocialUtils
  KAccounts
  ${ACCOUNTSQT_LIBRARIES}
  ${SIGNONQT_LIBRARIES}
)

add_subdirectory(icons)
add_subdirectory(serializer)

install(TARGETS akonadi_facebook_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES facebookresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(FILES akonadi_facebook_resource.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})