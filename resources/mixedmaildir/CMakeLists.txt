include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${kdepim-runtime_SOURCE_DIR}/resources/shared
    ${kdepim-runtime_SOURCE_DIR}/resources/shared/filestore
    ${kdepim-runtime_SOURCE_DIR}/resources/maildir
    ${kdepim-runtime_SOURCE_DIR}/resources/mbox
    ${CMAKE_CURRENT_SOURCE_DIR}/kmindexreader
    )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mixedmaildir_resource\")

add_subdirectory(kmindexreader)

set(mixedmaildir_common_SRCS)
kconfig_add_kcfg_files(mixedmaildir_common_SRCS settings.kcfgc)

############################# Resource #######################################

set( mixedmaildirresource_SRCS
    compactchangehelper.cpp
    mixedmaildirresource.cpp
    mixedmaildirstore.cpp
    retrieveitemsjob.cpp
    ${mixedmaildir_common_SRCS}
    )
ecm_qt_declare_logging_category(mixedmaildirresource_SRCS HEADER mixedmaildirresource_debug.h IDENTIFIER MIXEDMAILDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.mixedmaildirresource
    DESCRIPTION "mixed maildir resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_mixedmaildir
    EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(mixedmaildirresource_SRCS HEADER mixedmaildir_debug.h IDENTIFIER MIXEDMAILDIR_LOG CATEGORY_NAME org.kde.pim.mixedmaildir
    DESCRIPTION "mixedmaildir resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_mixedmaildirresource
    EXPORT KDEPIMRUNTIME
    )

install( FILES mixedmaildirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mixedmaildirresource.kcfg org.kde.Akonadi.MixedMaildir.Settings)

qt5_add_dbus_adaptor(mixedmaildirresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml settings.h Settings
    )

add_executable(akonadi_mixedmaildir_resource ${mixedmaildirresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_mixedmaildir_resource PROPERTIES UNITY_BUILD ON)
endif()
if( APPLE )
    set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.MixedMaildir")
    set_target_properties(akonadi_mixedmaildir_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi MixedMaildir Resource")
endif ()

target_link_libraries(akonadi_mixedmaildir_resource
    kmindexreader
    maildir
    akonadi-filestore
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::KIOCore
    KF5::Mbox
    KF5::Mime
    KF5::AkonadiAgentBase
    KF5::I18n
    akonadi-singlefileresource
    )
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_mixedmaildir_resource PROPERTIES UNITY_BUILD ON)
endif()

install(TARGETS akonadi_mixedmaildir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml
    DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

# unit tests
if (BUILD_TESTING)
    add_subdirectory( autotests )
endif ()

###################################### Config Plugin #########################

set(mixedmaildirconfig_SRCS
    mixedmaildirconfig.cpp
    configwidget.cpp
    ${mixedmaildir_common_SRCS}
    )
ki18n_wrap_ui(mixedmaildirconfig_SRCS settings.ui)

kcoreaddons_add_plugin(mixedmaildirconfig
    SOURCES ${mixedmaildirconfig_SRCS}
    JSON "mixedmaildirconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
    )
target_link_libraries(mixedmaildirconfig
    KF5::AkonadiCore
    KF5::I18n
    KF5::KIOWidgets
    KF5::ConfigWidgets
    maildir
    )
