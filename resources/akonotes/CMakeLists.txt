include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir
    ${CMAKE_CURRENT_BINARY_DIR}/../maildir
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir/libmaildir
)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_akonotes_resource\")

set(akonotes_common_SRCS)
kconfig_add_kcfg_files(akonotes_common_SRCS ../maildir/settings.kcfgc)

################################# Resource #################################

set( akonotesresource_SRCS
  ../maildir/maildirresource.cpp
  ../maildir/retrieveitemsjob.cpp
  ../maildir/maildirresource_debug.cpp
  akonotesresource.cpp
  ${akonotes_common_SRCS}
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt5_add_dbus_adaptor(akonotesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

install( FILES akonotesresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_akonotes_resource ${akonotesresource_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_akonotes_resource PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(akonadi_akonotes_resource
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::Mime
    KF5::AkonadiAgentBase
    KF5::I18n
    maildir
    folderarchivesettings
)

install(TARGETS akonadi_akonotes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############################### Config plugin #################################

set(akonotesconfig_SRCS
    ../maildir/configwidget.cpp
    ../maildir/maildirconfig.cpp
    akonotesconfig.cpp
    ${akonotes_common_SRCS}
)
ki18n_wrap_ui(akonotesconfig_SRCS ../maildir/settings.ui)

kcoreaddons_add_plugin(akonotesconfig
    SOURCES ${akonotesconfig_SRCS}
    JSON "akonotesconfig.json"
    INSTALL_NAMESPACE akonadi/config
)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonotesconfig PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(akonotesconfig
    KF5::AkonadiCore
    KF5::KIOWidgets
    KF5::ConfigWidgets
    KF5::I18n
    maildir
    folderarchivesettings
)
