# SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_davgroupware_resource\")

set(davgroupwareresource_common_SRCS
    davprotocolattribute.cpp
    davprotocolattribute.h
    settings.cpp
    settings.h
    utils.cpp
    utils.h
)

ecm_qt_declare_logging_category(davgroupwareresource_common_SRCS HEADER davresource_debug.h IDENTIFIER DAVRESOURCE_LOG CATEGORY_NAME org.kde.pim.davresource
    DESCRIPTION "dav resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_davresource
    EXPORT KDEPIMRUNTIME
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/davgroupwareresource.kcfg org.kde.Akonadi.davGroupware.Settings)
qt_add_dbus_adaptor(davgroupwareresource_common_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.davGroupware.Settings.xml settings.h Settings
)
kconfig_add_kcfg_files(davgroupwareresource_common_SRCS settingsbase.kcfgc)

###### next target #####

if(WIN32)
    set(LIB_INSTALL_DIR ${KDE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${KDE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${KDE_INSTALL_LIBDIR})
endif()

########### next target ###############

set(davgroupwareresource_SRCS
    ${davgroupwareresource_common_SRCS}
    daverror-kdepim-runtime.cpp
    akonadietagcache.cpp

    ctagattribute.cpp
    davfreebusyhandler.cpp
    davgroupwareresource.cpp
    daverror-kdepim-runtime.h
    akonadietagcache.h

    ctagattribute.h
    davfreebusyhandler.h
    davgroupwareresource.h
)

install(FILES davgroupwareresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(FILES org.kde.akonadi_davgroupware_resource.desktop DESTINATION ${KDE_INSTALL_APPDIR})

file(GLOB providersFiles "../services/*.desktop")
install(FILES ${providersFiles} DESTINATION "${KDE_INSTALL_DATADIR}/akonadi/davgroupware-providers")

add_executable(akonadi_davgroupware_resource ${davgroupwareresource_SRCS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES UNITY_BUILD ON)
endif()

if(APPLE)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template)
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.davGroupware")
    set_target_properties(akonadi_davgroupware_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi davGroupware Resource")
endif()

target_link_libraries(akonadi_davgroupware_resource
    Qt::Xml
    KF6::I18n
    KF6::Contacts
    KF6::KIOCore
    KF6::CalendarCore
    KF6::ConfigCore
    KF6::DAV
    KPim6::AkonadiAgentBase
    KPim6::AkonadiCore
    KPim6::AkonadiCalendarCore
    qt6keychain
)

install(TARGETS akonadi_davgroupware_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


################################## Config plugin ##############################
kcoreaddons_add_plugin(davgroupwareconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

target_sources(davgroupwareconfig PRIVATE
    ${davgroupwareconfig_SRCS}
    ${davgroupwareresource_common_SRCS}
    davgroupwareconfig.cpp
    urlconfigurationdialog.cpp
    urlconfigurationdialog.h
    configwidget.cpp
    searchdialog.cpp
    setupwizard.cpp
    configwidget.h
    searchdialog.h
    setupwizard.h
)

ki18n_wrap_ui(davgroupwareconfig configwidget.ui urlconfigurationdialog.ui searchdialog.ui)

target_link_libraries(davgroupwareconfig PUBLIC
    KF6::Completion
    KF6::WidgetsAddons
    KF6::I18n
    KF6::CalendarCore
    KF6::Notifications
    KF6::DAV
    KF6::Service
    KF6::Contacts
    KPim6::AkonadiCore
    KPim6::AkonadiWidgets
    KPim6::AkonadiAgentBase
    qt6keychain
)
