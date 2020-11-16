add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_pop3_resource\")


set(pop3_common_SRCS
    settings.cpp
    )
kconfig_add_kcfg_files(pop3_common_SRCS settingsbase.kcfgc)
ecm_qt_declare_logging_category(pop3_common_SRCS HEADER pop3resource_debug.h IDENTIFIER POP3RESOURCE_LOG CATEGORY_NAME org.kde.pim.pop3resource
    DESCRIPTION "pop3 resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_pop3resource
    EXPORT KDEPIMRUNTIME
    )

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt5_add_dbus_adaptor(pop3_common_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
    )

################################# Resource ####################################

set( pop3resource_SRCS
    pop3resource.cpp
    jobs.cpp
    ${pop3_common_SRCS}
    )

install( FILES pop3resource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

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
    KF5::I18n
    KF5::Notifications
    Qt5::DBus
    KF5::PimCommon
    qt5keychain
    )

if(BUILD_TESTING)
    add_subdirectory( autotests )
endif()

add_subdirectory( wizard )

install(TARGETS akonadi_pop3_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES akonadi_pop3_resource.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})


################################## Config plugin ##############################

set(pop3config_SRCS
    pop3config.cpp
    accountwidget.cpp
    ${pop3_common_SRCS}
    )
ki18n_wrap_ui(pop3config_SRCS popsettings.ui)

kcoreaddons_add_plugin(pop3config
    SOURCES ${pop3config_SRCS}
    JSON "pop3config.json"
    INSTALL_NAMESPACE "akonadi/config"
    )
target_link_libraries(pop3config
    KF5::AkonadiCore
    KF5::TextWidgets
    KF5::Completion
    KF5::WidgetsAddons
    KF5::I18n
    KF5::MailTransport
    KF5::AkonadiWidgets
    KF5::AkonadiAgentBase
    KF5::AkonadiMime
    KF5::Libkdepim
    qt5keychain
    )
