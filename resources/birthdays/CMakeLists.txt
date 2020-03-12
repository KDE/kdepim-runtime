add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_birthdays_resource\")

set(birthdayresource_common_SRCS)

kconfig_add_kcfg_files(birthdayresource_common_SRCS
    settings.kcfgc
    )

set( birthdayresource_srcs
    birthdaysresource.cpp
    ${birthdayresource_common_SRCS}
    )


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/birthdaysresource.kcfg org.kde.Akonadi.Birthdays.Settings)

qt5_add_dbus_adaptor(birthdayresource_srcs
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Birthdays.Settings.xml settings.h Settings
    )

ecm_qt_declare_logging_category(birthdayresource_srcs HEADER birthdays_debug.h IDENTIFIER BIRTHDAYS_LOG CATEGORY_NAME org.kde.pim.birthdays
    DESCRIPTION "birthdays resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_birthdays
    EXPORT KDEPIMRUNTIME
    )



add_executable(akonadi_birthdays_resource ${birthdayresource_srcs})

if( APPLE )
    set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Birthdays")
    set_target_properties(akonadi_birthdays_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Birthdays Resource")
endif ()

target_link_libraries(akonadi_birthdays_resource
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    )

install( TARGETS akonadi_birthdays_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES birthdaysresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )


############################# Config plugin ################################
set(birthdaysconfig_ui_SRCS)
ki18n_wrap_ui(birthdaysconfig_ui_SRCS birthdaysconfigwidget.ui)
set(birthdaysconfig_SRCS
    birthdaysconfigagentwidget.cpp
    ${birthdaysconfig_ui_SRCS}
    ${birthdayresource_common_SRCS}
    )


kcoreaddons_add_plugin(birthdaysconfig
    SOURCES ${birthdaysconfig_SRCS}
    JSON "birthdaysconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
    )
target_link_libraries(birthdaysconfig
    KF5::AkonadiCore
    KF5::CalendarCore
    KF5::Contacts
    KF5::AkonadiWidgets
    KF5::I18n
    KF5::TextWidgets
    )
