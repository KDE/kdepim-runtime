project(maildir)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libmaildir
    ${kdepim-runtime_SOURCE_DIR}
)


add_subdirectory( autotests )
add_subdirectory( wizard )

# maildir access library
add_subdirectory(libmaildir)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_maildir_resource\")

########### next target ###############
set( maildirresource_SRCS
  maildirresource.cpp
  configdialog.cpp
  retrieveitemsjob.cpp
  maildirresourcefactory.cpp
)

ki18n_wrap_ui(maildirresource_SRCS settings.ui)

kconfig_add_kcfg_files(maildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt5_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

add_library(akonadi_maildir_resource MODULE ${maildirresource_SRCS})

target_link_libraries(akonadi_maildir_resource KF5::AkonadiCore KF5::AkonadiMime maildir  KF5::KIOCore KF5::Mime folderarchivesettings KF5::AkonadiAgentBase)

install(TARGETS akonadi_maildir_resource DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

install( FILES maildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

