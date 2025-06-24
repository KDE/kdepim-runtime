add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_etesync_resource\")

set(etesyncresource_common_SRCS
    etebaseadapter.cpp etebaseadapter.h
)

kconfig_add_kcfg_files(etesyncresource_common_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfgc
)

set(etesyncresource_SRCS
    etesyncresource.cpp etesyncresource.h
    etesyncclientstate.cpp etesyncclientstate.h
    journalsfetchjob.cpp journalsfetchjob.h
    entriesfetchjob.cpp entriesfetchjob.h
    ${etesyncresource_common_SRCS}
)

ecm_qt_declare_logging_category(etesyncresource_SRCS
    HEADER etesync_debug.h
    IDENTIFIER ETESYNC_LOG
    CATEGORY_NAME org.kde.pim.etesync
    DESCRIPTION "resource etesync (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/etesyncresource.kcfg
    org.kde.Akonadi.etesync.Settings
)

qt_add_dbus_adaptor(etesyncresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.etesync.Settings.xml
    ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    Settings
)

add_executable(akonadi_etesync_resource ${etesyncresource_SRCS})
set_target_properties(akonadi_etesync_resource PROPERTIES MACOSX_BUNDLE FALSE)
target_include_directories(akonadi_etesync_resource PRIVATE ${ETEBASE_INCLUDE_DIRS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_etesync_resource PROPERTIES UNITY_BUILD ON)
endif()
target_link_libraries(akonadi_etesync_resource
    ${ETEBASE_LIBRARIES}
    Qt::DBus
    Qt::Concurrent
    KPim6::AkonadiAgentBase
    KF6::ConfigCore
    KF6::Contacts
    KF6::CalendarCore
    KF6::KIOCore
    KF6::I18n
    KF6::Notifications
    qt6keychain
)

ecm_install_icons(
    ICONS
    icons/16-apps-akonadi-etesync.png
    icons/24-apps-akonadi-etesync.png
    icons/32-apps-akonadi-etesync.png
    icons/48-apps-akonadi-etesync.png
    icons/64-apps-akonadi-etesync.png
    icons/72-apps-akonadi-etesync.png
    icons/128-apps-akonadi-etesync.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

install(FILES akonadi_etesync_resource.notifyrc
        DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

install(TARGETS akonadi_etesync_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES etesyncresource.desktop
    DESTINATION ${KDE_INSTALL_DATAROOTDIR}/akonadi/agents
)

################################## Config plugin ##############################
kcoreaddons_add_plugin(etesyncconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

set(etesyncconfig_CONFIG_SRC 
    etesyncconfig.cpp
    setupwizard.cpp setupwizard.h
    loginjob.cpp loginjob.h
    etesyncclientstate.cpp etesyncclientstate.h
)
ecm_qt_declare_logging_category(etesyncconfig_CONFIG_SRC
    HEADER etesync_debug.h      
    IDENTIFIER ETESYNC_LOG      
    CATEGORY_NAME org.kde.pim.etesync 
    DESCRIPTION "EteSync Config Plugin Logging"
)

target_sources(etesyncconfig PRIVATE
    ${etesyncconfig_CONFIG_SRC}
    ${etesyncresource_common_SRCS}
)

target_include_directories(etesyncconfig PRIVATE ${ETEBASE_INCLUDE_DIRS})

target_link_libraries(etesyncconfig PUBLIC
    ${ETEBASE_LIBRARIES}
    Qt::Concurrent
    KF6::ConfigCore
    KF6::ConfigWidgets
    KF6::CoreAddons
    KF6::I18n
    KF6::WidgetsAddons
    KF6::WindowSystem
    KPim6::AkonadiCore
    KPim6::AkonadiWidgets
    KPim6::AkonadiAgentWidgetBase
    qt6keychain
)
