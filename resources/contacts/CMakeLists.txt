kde_enable_exceptions()

add_subdirectory( wizard )

########### next target ###############

set( contactsresource_SRCS
  contactsresource.cpp
  settingsdialog.cpp
)

qt5_wrap_ui(contactsresource_SRCS settingsdialog.ui)
kconfig_add_kcfg_files(contactsresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/contactsresource.kcfg org.kde.Akonadi.Contacts.Settings)
qt5_add_dbus_adaptor(contactsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Contacts.Settings.xml settings.h Akonadi_Contacts_Resource::ContactsResourceSettings contactsresourcesettingsadaptor ContactsResourceSettingsAdaptor
)

install( FILES contactsresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

add_library(akonadi_contacts_resource MODULE ${contactsresource_SRCS})

target_link_libraries(akonadi_contacts_resource
  KF5::AkonadiCore
  KF5::AkonadiAgentBase
  KF5::Abc
  KF5::KIOCore
)

install(TARGETS akonadi_contacts_resource DESTINATION ${PLUGIN_INSTALL_DIR})
