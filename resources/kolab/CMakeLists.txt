add_subdirectory(pimkolab)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${CMAKE_CURRENT_SOURCE_DIR}/pimkolab/
    ${Libkolabxml_INCLUDES}
    )

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")


########### next target ###############

set(kolabresource_SRCS
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
    )

ecm_qt_declare_logging_category(kolabresource_SRCS
    HEADER kolabresource_debug.h
    IDENTIFIER KOLABRESOURCE_LOG
    CATEGORY_NAME org.kde.pim.kolabresource
    DESCRIPTION "kolab resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_kolabresource
    EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(kolabresource_SRCS
    HEADER kolabresource_trace.h
    IDENTIFIER KOLABRESOURCE_TRACE
    CATEGORY_NAME org.kde.pim.kolabresource.trace
    DESCRIPTION "resource kolab trace (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


kconfig_add_kcfg_files(kolabresource_SRCS ../imap/settingsbase.kcfgc)

ki18n_wrap_ui(kolabresource_SRCS ../imap/setupserverview_desktop.ui)
add_executable(akonadi_kolab_resource ${kolabresource_SRCS})

target_link_libraries(akonadi_kolab_resource
    KF5::AkonadiCore
    KF5::IMAP
    KF5::MailTransport
    KF5::Mime
    KF5::AkonadiMime
    KF5::AkonadiCalendar
    KF5::AkonadiWidgets
    KF5::IdentityManagement
    imapresource
    folderarchivesettings
    pimkolab
    KF5::Contacts
    KF5::CalendarCore
    akonadi-singlefileresource
    KF5::PimCommon
    KF5::TextWidgets
    KF5::I18n
    )

install(FILES kolabresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if (BUILD_TESTING)
    #add_subdirectory(autotests)
endif()
add_subdirectory(wizard)
