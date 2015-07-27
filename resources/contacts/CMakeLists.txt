

add_subdirectory( wizard )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_contacts_resource\")

########### next target ###############

set( contactsresource_SRCS
  contactsresource.cpp
  settingsdialog.cpp
  contacts_resources_debug.cpp
)

ki18n_wrap_ui(contactsresource_SRCS settingsdialog.ui)
kconfig_add_kcfg_files(contactsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/contactsresource.kcfg org.kde.Akonadi.Contacts.Settings)
qt5_add_dbus_adaptor(contactsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Contacts.Settings.xml settings.h Akonadi_Contacts_Resource::ContactsResourceSettings contactsresourcesettingsadaptor ContactsResourceSettingsAdaptor
)

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
