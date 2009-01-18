include_directories(
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( birthdayresource_srcs
  birthdaysresource.cpp
  configdialog.cpp
)

kde4_add_kcfg_files( birthdayresource_srcs settings.kcfgc )
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/birthdaysresource.kcfg org.kde.Akonadi.Birthdays.Settings)
kde4_add_ui_files(birthdayresource_srcs configdialog.ui)
qt4_add_dbus_adaptor(birthdayresource_srcs
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Birthdays.Settings.xml settings.h Settings
)
kde4_add_executable(akonadi_birthdays_resource RUN_UNINSTALLED ${birthdayresource_srcs})

target_link_libraries(akonadi_birthdays_resource
  ${KDE4_KIO_LIBS}
  ${KDE4_AKONADI_LIBS}
  ${KDE4_KABC_LIBS}
  ${KDE4_KCAL_LIBS}
)

install( TARGETS akonadi_birthdays_resource ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES birthdaysresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
