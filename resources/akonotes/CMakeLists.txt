include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir
    ${CMAKE_CURRENT_BINARY_DIR}/../maildir
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir/libmaildir
    ${Boost_INCLUDE_DIR}
)




########### next target ###############

set( akonotesresource_SRCS
  ../maildir/maildirresource.cpp
  ../maildir/configdialog.cpp
  ../maildir/retrieveitemsjob.cpp
  akonotesresource.cpp
  akonotesresourcefactory.cpp
)


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

ki18n_wrap_ui(akonotesresource_SRCS ../maildir/settings.ui)

kconfig_add_kcfg_files(akonotesresource_SRCS ../maildir/settings.kcfgc)

qt5_add_dbus_adaptor(akonotesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

install( FILES akonotesresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

add_library(akonadi_akonotes_resource MODULE ${akonotesresource_SRCS})

target_link_libraries(akonadi_akonotes_resource KF5::AkonadiCore KF5::AkonadiMime maildir  KF5::KIOCore KF5::Mime folderarchivesettings KF5::AkonadiAgentBase)

install(TARGETS akonadi_akonotes_resource DESTINATION ${PLUGIN_INSTALL_DIR})

