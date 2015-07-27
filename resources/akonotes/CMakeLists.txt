include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir
    ${CMAKE_CURRENT_BINARY_DIR}/../maildir
    ${CMAKE_CURRENT_SOURCE_DIR}/../maildir/libmaildir
    ${Boost_INCLUDE_DIR}
)


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_akonotes_resource\")


########### next target ###############

set( akonotesresource_SRCS
  ../maildir/maildirresource.cpp
  ../maildir/configdialog.cpp
  ../maildir/retrieveitemsjob.cpp
  ../maildir/maildirresource_debug.cpp
  akonotesresource.cpp
)


kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)

ki18n_wrap_ui(akonotesresource_SRCS ../maildir/settings.ui)

kconfig_add_kcfg_files(akonotesresource_SRCS ../maildir/settings.kcfgc)

qt5_add_dbus_adaptor(akonotesresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Akonadi_Maildir_Resource::MaildirSettings maildirsettingsadaptor MaildirSettingsAdaptor
)

install( FILES akonotesresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )

add_executable(akonadi_akonotes_resource ${akonotesresource_SRCS})

target_link_libraries(akonadi_akonotes_resource
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::Mime
    KF5::AkonadiAgentBase
    KF5::DBusAddons
    KF5::I18n
    KF5::KIOWidgets
    KF5::WindowSystem
    KF5::ConfigWidgets
    maildir
    folderarchivesettings
)

install(TARGETS akonadi_akonotes_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

