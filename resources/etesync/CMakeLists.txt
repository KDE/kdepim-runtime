set(etesyncresource_SRCS
    etesyncresource.cpp
)

ecm_qt_declare_logging_category(etesyncresource_SRCS
    HEADER debug.h
    IDENTIFIER log_etesyncresource
    CATEGORY_NAME log_etesyncresource
)

kconfig_add_kcfg_files(etesyncresource_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfgc
)

kcfg_generate_dbus_interface(
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg
    org.kde.Akonadi.etesync.Settings
)

qt5_add_dbus_adaptor(etesyncresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.etesync.Settings.xml
    ${CMAKE_CURRENT_BINARY_DIR}/settings.h
    Settings
)

add_executable(akonadi_etesync_resource ${etesyncresource_SRCS})
set_target_properties(akonadi_etesync_resource PROPERTIES MACOSX_BUNDLE FALSE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ETESYNC REQUIRED etesync)

target_link_libraries(akonadi_etesync_resource
    ${ETESYNC_LIBRARIES}
    Qt5::DBus
    KF5::AkonadiAgentBase
    KF5::ConfigCore
)

install(TARGETS akonadi_etesync_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES etesyncresource.desktop
    DESTINATION ${KDE_INSTALL_DATAROOTDIR}/akonadi/agents
)
