include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${Libkolabxml_INCLUDES}
    ${Libkolab_INCLUDES}
)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")


########### next target ###############

set(kolabresource_SRCS
    ../imap/imapresource.cpp
    ../imap/settingspasswordrequester.cpp
    ../imap/setupserver.cpp
    ../imap/serverinfodialog.cpp
    kolabretrievecollectionstask.cpp
    kolabresource.cpp
    kolabresourcestate.cpp
    kolabhelpers.cpp
    kolabmessagehelper.cpp
    kolabaddtagtask.cpp
    kolabchangetagtask.cpp
    kolabremovetagtask.cpp
    kolabretrievetagstask.cpp
    kolabsettings.cpp
    tagchangehelper.cpp
    kolabrelationresourcetask.cpp
    kolabchangeitemsrelationstask.cpp
    kolabchangeitemstagstask.cpp
    updatemessagejob.cpp
    kolabresource_debug.cpp
)

kconfig_add_kcfg_files(kolabresource_SRCS ../imap/settingsbase.kcfgc)

if (KDEPIM_MOBILE_UI)
ki18n_wrap_ui(kolabresource_SRCS ../imap/setupserverview_mobile.ui)
else ()
ki18n_wrap_ui(kolabresource_SRCS ../imap/setupserverview_desktop.ui)
endif ()
ki18n_wrap_ui(kolabresource_SRCS ../imap/serverinfo.ui)
add_executable(akonadi_kolab_resource ${kolabresource_SRCS})

target_link_libraries(akonadi_kolab_resource
    KF5::AkonadiCore
    KF5::IMAP
    KF5::MailTransport
    KF5::KIOCore
    KF5::Mime
    KF5::AkonadiMime
    KF5::AkonadiCalendar
    KF5::IdentityManagement
    imapresource
    folderarchivesettings
    ${Libkolabxml_LIBRARIES}
    ${Libkolab_LIBRARIES}
    KF5::Contacts
    KF5::CalendarCore
    akonadi-singlefileresource
)

install(FILES kolabresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if (BUILD_TESTING)
    #add_subdirectory(autotests)
endif()
