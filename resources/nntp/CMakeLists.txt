
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)
set( nntpresource_SRCS
  nntpcollectionattribute.cpp
  nntpresource.cpp
  configdialog.cpp
  settings.cpp
)

install( FILES nntpresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/nntpresource.kcfg org.kde.Akonadi.NNTP.Settings )

qt5_add_dbus_adaptor( nntpresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.NNTP.Settings.xml settings.h Settings
)
ki18n_wrap_ui(nntpresource_SRCS configdialog.ui)
kconfig_add_kcfg_files(nntpresource_SRCS settingsbase.kcfgc)
add_executable(akonadi_nntp_resource ${nntpresource_SRCS})


if (Q_WS_MAC)
  set_target_properties(akonadi_nntp_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_nntp_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.NNTP")
  set_target_properties(akonadi_nntp_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi NNTP Resource")
endif ()

target_link_libraries(akonadi_nntp_resource KF5::AkonadiCore KF5::AkonadiMime KF5::KIOCore KF5::Mime KF5::AkonadiAgentBase)

install(TARGETS akonadi_nntp_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
