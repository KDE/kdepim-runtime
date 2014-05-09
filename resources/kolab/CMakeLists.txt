include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${Libkolab_INCLUDES}
    ${Libkolabxml_INCLUDES}
)

kde_enable_exceptions()
find_package(KF5Imap ${KF5_VERSION} CONFIG REQUIRED)

########### next target ###############

set(kolabresource_SRCS
    ../imap/imapidlemanager.cpp
    ../imap/imapresource.cpp
    ../imap/resourcestate.cpp
    ../imap/settings.cpp
    ../imap/settingspasswordrequester.cpp
    ../imap/setupserver.cpp
    ../imap/serverinfodialog.cpp
    ../imap/subscriptiondialog.cpp
    kolabretrievecollectionstask.cpp
    kolabresource.cpp
    kolabresourcestate.cpp
    kolabhelpers.cpp
    kolabmessagehelper.cpp
)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../imap/imapresource.kcfg org.kde.Akonadi.Imap.Settings)
qt5_add_dbus_adaptor(kolabresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
)
qt5_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../imap/imapresource.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt5_add_dbus_adaptor(kolabresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
    imapresource.h ImapResource
)

kconfig_add_kcfg_files(kolabresource_SRCS ../imap/settingsbase.kcfgc)

if (KDEPIM_MOBILE_UI)
kde4_add_ui_files(kolabresource_SRCS ../imap/setupserverview_mobile.ui)
else ()
kde4_add_ui_files(kolabresource_SRCS ../imap/setupserverview_desktop.ui)
endif ()
kde4_add_ui_files(kolabresource_SRCS ../imap/serverinfo.ui)
add_executable(akonadi_kolab_resource ${kolabresource_SRCS})

target_link_libraries(akonadi_kolab_resource
    KF5::AkonadiCore
    KF5::Imap    
    KF5::MailTransport
    KF5::KIOCore
    KF5::Mime
    KF5::AkonadiMime
    KF5::PimIdentities
    imapresource
    folderarchivesettings
    ${Libkolab_LIBRARIES}
    ${Libkolabxml_LIBRARIES}
    KF5::Abc
    KF5::CalendarCore
)

install(FILES kolabresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
