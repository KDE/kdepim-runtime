
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libmaildir
    ${kdepim-runtime_SOURCE_DIR}
)


if (BUILD_TESTING)
   add_subdirectory( autotests )
endif()
add_subdirectory( wizard )

# maildir access library
add_subdirectory(libmaildir)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildir_resource\")

set(maildir_common_SRCS)
kconfig_add_kcfg_files(maildir_common_SRCS settings.kcfgc)

############################### Resource #####################################
set( maildirresource_SRCS
  main.cpp
  maildirresource.cpp
  retrieveitemsjob.cpp
  ${maildir_common_SRCS}
)
ecm_qt_declare_logging_category(maildirresource_SRCS HEADER maildirresource_debug.h IDENTIFIER MAILDIRRESOURCE_LOG CATEGORY_NAME org.kde.pim.maildirresource)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt5_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

add_executable(akonadi_maildir_resource ${maildirresource_SRCS})

target_link_libraries(akonadi_maildir_resource
    maildir
    folderarchivesettings
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::Mime
    KF5::AkonadiAgentBase
    KF5::DBusAddons
    KF5::I18n
)

install(TARGETS akonadi_maildir_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

install( FILES maildirresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

############################# Config plugin ###############################

set(maildirconfig_SRCS
    maildirconfigmain.cpp
    maildirconfig.cpp
    configwidget.cpp
    ${maildir_common_SRCS}
)

kcoreaddons_add_plugin(maildirconfig
    SOURCES ${maildirconfig_SRCS}
    JSON "maildirconfig.json"
    INSTALL_NAMESPACE "akonadi/config"
)
target_link_libraries(maildirconfig
    KF5::AkonadiCore
    KF5::ConfigWidgets
    KF5::KIOWidgets
    KF5::I18n
    folderarchivesettings
    maildir
)
