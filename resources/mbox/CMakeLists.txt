
set(mboxresource_common_SRCS
    deleteditemsattribute.cpp
    deleteditemsattribute.h
)
kconfig_add_kcfg_files(mboxresource_common_SRCS settings.kcfgc)

################################## Resource #################################
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_mbox_resource\")

set(mboxresource_SRCS
    mboxresource.cpp
    mboxresource.h
    ${mboxresource_common_SRCS}
)

# mboxresource.cpp needs UI files generated for another target. We must be sure the files
# were created before building the akonadi_mbox_resource target.
add_custom_target(generated_headers
    DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/ui_compactpage.h
    ${CMAKE_CURRENT_BINARY_DIR}/ui_lockfilepage.h
)
set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/ui_compactpage.h
    ${CMAKE_CURRENT_BINARY_DIR}/ui_lockfilepage.h
    PROPERTIES GENERATED TRUE
)

ecm_qt_declare_logging_category(mboxresource_SRCS HEADER mboxresource_debug.h IDENTIFIER MBOXRESOURCE_LOG CATEGORY_NAME org.kde.pim.mboxresource
    DESCRIPTION "mbox resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_mboxresource
    EXPORT KDEPIMRUNTIME
)


install(FILES mboxresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mboxresource.kcfg org.kde.Akonadi.Mbox.Settings)
qt_add_dbus_adaptor(mboxresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Mbox.Settings.xml settings.h Settings
)

add_executable(akonadi_mbox_resource ${mboxresource_SRCS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_mbox_resource PROPERTIES UNITY_BUILD ON)
endif()
add_dependencies(akonadi_mbox_resource generated_headers)

if(APPLE)
    set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
    set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Mbox")
    set_target_properties(akonadi_mbox_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Mbox Resource")
endif()

target_link_libraries(akonadi_mbox_resource
    KF6::I18n
    KPim6::Mbox
    KPim6::AkonadiCore
    KPim6::AkonadiMime
    KF6::KIOCore
    KPim6::Mime
    KPim6::AkonadiAgentBase
    akonadi-singlefileresource
)

install(TARGETS akonadi_mbox_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

############################# Config plugin ################################
kcoreaddons_add_plugin(mboxconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

target_sources(mboxconfig PRIVATE
    mboxconfig.cpp
    lockmethodpage.cpp
    compactpage.cpp
    lockmethodpage.h
    compactpage.h
    ${mboxresource_common_SRCS}
)

ki18n_wrap_ui(mboxconfig
    compactpage.ui
    lockfilepage.ui
)

target_link_libraries(mboxconfig
    KPim6::AkonadiCore
    KF6::ConfigWidgets
    KF6::KIOWidgets
    KF6::I18n
    KPim6::Mbox
    akonadi-singlefileresource-widget
)
if(HAVE_ACTIVITY_SUPPORT)
    target_link_libraries(mboxconfig
        KPim6::PimCommonActivities
    )
endif()

