project(pop3)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pop3_resource\")



########### next target ###############

set( pop3resource_SRCS
  pop3resource.cpp
  accountdialog.cpp
  jobs.cpp
  settings.cpp
  pop3resource_debug.cpp
)



install( FILES pop3resource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

ki18n_wrap_ui( pop3resource_SRCS popsettings.ui)
kconfig_add_kcfg_files(pop3resource_SRCS settingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt5_add_dbus_adaptor(pop3resource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
)

#add_executable(akonadi_pop3_resource RUN_UNINSTALLED ${pop3resource_SRCS})
add_executable(akonadi_pop3_resource ${pop3resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.POP3")
  set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi POP3 Resource")
endif ()


target_link_libraries( akonadi_pop3_resource
  KF5::AkonadiCore
  KF5::AkonadiMime
  KF5::KIOCore
  KF5::Mime
  KF5::MailTransport
  KF5::AkonadiAgentBase
  KF5::AkonadiWidgets
)

add_subdirectory( autotests )
add_subdirectory( wizard )

install(TARGETS akonadi_pop3_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
