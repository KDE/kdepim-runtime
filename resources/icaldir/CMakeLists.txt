include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

kde_enable_exceptions()


########### next target ###############

set( icaldirresource_SRCS
  icaldirresource.cpp
  ../shared/dirsettingsdialog.cpp
)

qt5_wrap_ui(icaldirresource_SRCS settingsdialog.ui)
kconfig_add_kcfg_files(icaldirresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/icaldirresource.kcfg org.kde.Akonadi.ICalDirectory.Settings)
qt5_add_dbus_adaptor(icaldirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.ICalDirectory.Settings.xml settings.h Settings
)

install( FILES icaldirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

add_executable(akonadi_icaldir_resource ${icaldirresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.ICalDirectory")
  set_target_properties(akonadi_icaldir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi ICalDirectory Resource")
endif ()


target_link_libraries(akonadi_icaldir_resource
  KF5::AkonadiCore
  
  ${KDE4_KDECORE_LIBS}
  KF5::CalendarCore
  KF5::KIOCore
)

install(TARGETS akonadi_icaldir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
