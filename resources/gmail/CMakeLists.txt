# REACTIVATE IT
#add_subdirectory(saslplugin)
add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_gmail_resource\")


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${CMAKE_CURRENT_SOURCE_DIR}/../folderarchivesettings/
)



########### next target ###############

set(gmailresource_SRCS
    gmailresource.cpp
    gmailretrievecollectionstask.cpp
    gmailretrieveitemstask.cpp
    gmailmessagehelper.cpp
    gmailpasswordrequester.cpp
    gmailresourcestate.cpp
    gmailconfigdialog.cpp
    gmailsettings.cpp
    gmaillinkitemstask.cpp
    gmaillabelattribute.cpp
    gmailchangeitemslabelstask.cpp
)

ki18n_wrap_ui(gmailresource_SRCS gmailconfigdialog.ui)
#kconfig_add_kcfg_files(gmailresource_SRCS settingsbase.kcfgc)

add_executable(akonadi_gmail_resource ${gmailresource_SRCS})
target_link_libraries(akonadi_gmail_resource
    KF5::AkonadiCore
    KF5::IMAP
    KF5::MailTransport
    KF5::KIOCore
    KF5::Mime
    KF5::AkonadiMime
    KF5::IdentityManagement
    KF5::GAPICore
    imapresource
    folderarchivesettings
)

install(FILES gmailresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(TARGETS akonadi_gmail_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
