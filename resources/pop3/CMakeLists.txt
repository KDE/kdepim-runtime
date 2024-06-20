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
ecm_qt_declare_logging_category(pop3_common_SRCS HEADER pop3protocol_debug.h IDENTIFIER POP3_LOG CATEGORY_NAME org.kde.pim.pop3protocol
    DESCRIPTION "pop3 protocol (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg org.kde.Akonadi.POP3.Settings)
qt_add_dbus_adaptor(pop3_common_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml settings.h Settings
)

################################# Resource ####################################

set(pop3resource_SRCS
    pop3resource.cpp
    pop3protocol.cpp
    jobs.cpp
    pop3resource.h
    pop3protocol.h
    jobs.h
    ${pop3_common_SRCS}
)

install(FILES pop3resource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

#add_executable(akonadi_pop3_resource RUN_UNINSTALLED ${pop3resource_SRCS})
add_executable(akonadi_pop3_resource ${pop3resource_SRCS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_pop3_resource PROPERTIES UNITY_BUILD ON)
endif()
if(APPLE)
    set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.POP3")
    set_target_properties(akonadi_pop3_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi POP3 Resource")
endif()


target_link_libraries(akonadi_pop3_resource
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KPim6::Mime
    KPim6::MailTransport
    KPim6::AkonadiAgentBase
    KF6::I18n
    KF6::KIOWidgets  # SslUi
    KF6::Notifications
    Qt::DBus
    Qt::Network # QNetworkProxy
    qt6keychain
    KF6::WidgetsAddons
    Sasl2::Sasl2
)
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_pop3_resource PROPERTIES UNITY_BUILD ON)
endif()

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

install(TARGETS akonadi_pop3_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES akonadi_pop3_resource.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})


################################## Config plugin ##############################
kcoreaddons_add_plugin(pop3config
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

target_sources(pop3config PRIVATE
    pop3config.cpp
    accountwidget.cpp
    accountwidget.h
    ${pop3_common_SRCS}
)
ki18n_wrap_ui(pop3config popsettings.ui)

target_link_libraries(pop3config
    KPim6::AkonadiCore
    KF6::TextWidgets
    KF6::Completion
    KF6::WidgetsAddons
    KF6::I18n
    KPim6::MailTransport
    KPim6::AkonadiWidgets
    KPim6::AkonadiAgentBase
    KPim6::AkonadiMime
    qt6keychain
)
