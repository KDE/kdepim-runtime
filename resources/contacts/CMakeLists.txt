

add_subdirectory( wizard )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_contacts_resource\")


set(contactsresource_common_SRCS)

kconfig_add_kcfg_files(contactsresource_common_SRCS
    settings.kcfgc
    )
########### next target ###############

set( contactsresource_SRCS
    contactsresource.cpp
    ${contactsresource_common_SRCS}
    )

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/contactsresource.kcfg org.kde.Akonadi.Contacts.Settings)
qt5_add_dbus_adaptor(contactsresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Contacts.Settings.xml settings.h ContactsResourceSettings contactsresourcesettingsadaptor ContactsResourceSettingsAdaptor
    )

ecm_qt_declare_logging_category(contactsresource_SRCS HEADER contacts_resources_debug.h IDENTIFIER CONTACTSRESOURCES_LOG CATEGORY_NAME org.kde.pim.resources_contacts
    DESCRIPTION "contacts resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_resources_contacts
    EXPORT KDEPIMRUNTIME
    )


install( FILES contactsresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_contacts_resource ${contactsresource_SRCS})

target_link_libraries(akonadi_contacts_resource
    KF5::AkonadiCore
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::I18n
    KF5::KIOWidgets
    KF5::ConfigWidgets
    )

install(TARGETS akonadi_contacts_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################## Config plugin ################################
set(contactsconfig_ui_SRCS)
ki18n_wrap_ui(contactsconfig_ui_SRCS contactsagentsettingswidget.ui)

set(contactsconfig_SRCS
    contactssettingswidget.cpp
    ${contactsconfig_ui_SRCS}
    ${contactsresource_common_SRCS}
    )


kcoreaddons_add_plugin(contactsconfig
    SOURCES ${contactsconfig_SRCS}
    JSON "contactsconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
    )
target_link_libraries(contactsconfig
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::Codecs
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    KF5::KIOWidgets
    )
