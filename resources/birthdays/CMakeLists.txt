include_directories( ${Boost_INCLUDE_DIR} )

kde_enable_exceptions()

set( birthdayresource_srcs
  birthdaysresource.cpp
  configdialog.cpp
)

kconfig_add_kcfg_files( birthdayresource_srcs settings.kcfgc )
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/birthdaysresource.kcfg org.kde.Akonadi.Birthdays.Settings)

qt5_wrap_ui(birthdayresource_srcs configdialog.ui)

qt5_add_dbus_adaptor(birthdayresource_srcs
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Birthdays.Settings.xml settings.h Settings
)
add_executable(akonadi_birthdays_resource ${birthdayresource_srcs})

if (Q_WS_MAC)
  set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Birthdays")
  set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Birthdays Resource")
endif ()

target_link_libraries(akonadi_birthdays_resource
  ${KDE4_KIO_LIBS}
  KF5::AkonadiCore
  ${KDEPIMLIBS_KABC_LIBS}
  KF5::CalendarCore
  KF5::PimUtils
)

install( TARGETS akonadi_birthdays_resource ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES birthdaysresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
