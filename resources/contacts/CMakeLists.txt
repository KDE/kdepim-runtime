

add_subdirectory( wizard )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_contacts_resource\")

########### next target ###############

set( contactsresource_SRCS
  contactsresource.cpp
  settingsdialog.cpp
)

ki18n_wrap_ui(contactsresource_SRCS settingsdialog.ui)
kconfig_add_kcfg_files(contactsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/contactsresource.kcfg org.kde.Akonadi.Contacts.Settings)
qt5_add_dbus_adaptor(contactsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Contacts.Settings.xml settings.h Akonadi_Contacts_Resource::ContactsResourceSettings contactsresourcesettingsadaptor ContactsResourceSettingsAdaptor
)

ecm_qt_declare_logging_category(contactsresource_SRCS HEADER contacts_resources_debug.h IDENTIFIER CONTACTSRESOURCES_LOG CATEGORY_NAME org.kde.pim.resources_contacts)

install( FILES contactsresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_contacts_resource ${contactsresource_SRCS})

target_link_libraries(akonadi_contacts_resource
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::Contacts
  KF5::DBusAddons
  KF5::I18n
  KF5::KIOWidgets
  KF5::ConfigWidgets
  KF5::WindowSystem
)

install(TARGETS akonadi_contacts_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################## Config plugin ################################
#set(contactsconfig_ui_SRCS)
#ki18n_wrap_ui(contactsconfig_ui_SRCS birthdaysconfigwidget.ui)
#set(birthdaysconfig_SRCS
#    birthdaysconfigagentwidget.cpp
#    ${birthdaysconfig_ui_SRCS}
#    ${birthdayresource_common_SRCS}
#    )


#kcoreaddons_add_plugin(contactsconfig
#    SOURCES ${contactsconfig_ui_SRCS}
#    JSON "birthdaysconfig.json"
#    INSTALL_NAMESPACE "akonadi/config"
#    )
#target_link_libraries(contactsconfig
#    KF5::AkonadiCore
#    KF5::CalendarCore
#    KF5::Codecs
#    KF5::AkonadiAgentBase
#    KF5::Contacts
#    KF5::AkonadiWidgets
#    KF5::I18n
#    KF5::TextWidgets
#    KF5::WindowSystem
#    )
