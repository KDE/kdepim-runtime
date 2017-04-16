add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pop3_resource\")



########### next target ###############

set( pop3resource_SRCS
  pop3resource.cpp
  accountdialog.cpp
  jobs.cpp
  settings.cpp
)

ecm_qt_declare_logging_category(pop3resource_SRCS HEADER pop3resource_debug.h IDENTIFIER POP3RESOURCE_LOG CATEGORY_NAME org.kde.pim.pop3resource)

install( FILES pop3resource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

ki18n_wrap_ui( pop3resource_SRCS popsettings.ui)
kconfig_add_kcfg_files(pop3resource_SRCS settingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt5_add_dbus_adaptor(pop3resource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
)

#add_executable(akonadi_pop3_resource RUN_UNINSTALLED ${pop3resource_SRCS})
add_executable(akonadi_pop3_resource ${pop3resource_SRCS})

if( APPLE )
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
  KF5::I18n
  KF5::TextWidgets
  KF5::Completion
  KF5::WidgetsAddons
  KF5::WindowSystem
  KF5::Notifications
  Qt5::DBus
)

if(BUILD_TESTING)
    add_subdirectory( autotests )
endif()

add_subdirectory( wizard )

install(TARGETS akonadi_pop3_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES akonadi_pop3_resource.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
