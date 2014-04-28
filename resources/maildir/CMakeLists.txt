kde_enable_exceptions()

#reactivate it
#add_subdirectory( tests )
add_subdirectory( wizard )

# maildir access library
add_subdirectory(libmaildir)


########### next target ###############

set( maildirresource_SRCS
  maildirresource.cpp
  configdialog.cpp
  main.cpp
  retrieveitemsjob.cpp
)

install( FILES maildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt5_wrap_ui(maildirresource_SRCS settings.ui)

kconfig_add_kcfg_files(maildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

qt5_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

add_library(akonadi_maildir_resource MODULE ${maildirresource_SRCS})

target_link_libraries(akonadi_maildir_resource KF5::AkonadiCore KF5::AkonadiMime maildir  KF5::KIOCore KF5::Mime folderarchivesettings KF5::AkonadiAgentBase)

install(TARGETS akonadi_maildir_resource DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
