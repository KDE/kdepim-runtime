add_subdirectory(pimkolab)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${CMAKE_CURRENT_SOURCE_DIR}/pimkolab/
    ${Libkolabxml_INCLUDES}
)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")

set(kolabresource_common_SRCS
    ../imap/settings.cpp
    ../imap/settings.h
    ../imap/imapaccount.cpp
    ../imap/imapaccount.h
)

ecm_qt_declare_logging_category(kolabresource_common_SRCS
    HEADER kolabresource_debug.h
    IDENTIFIER KOLABRESOURCE_LOG
    CATEGORY_NAME org.kde.pim.kolabresource
    DESCRIPTION "kolab resource (kdepim-runtime)"
    OLD_CATEGORY_NAMES log_kolabresource
    EXPORT KDEPIMRUNTIME
)
ecm_qt_declare_logging_category(kolabresource_common_SRCS
    HEADER kolabresource_trace.h
    IDENTIFIER KOLABRESOURCE_TRACE
    CATEGORY_NAME org.kde.pim.kolabresource.trace
    DESCRIPTION "resource kolab trace (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)


kconfig_add_kcfg_files(kolabresource_common_SRCS ../imap/settingsbase.kcfgc)

########### next target ###############

set(kolabresource_SRCS
    ../imap/settingspasswordrequester.cpp
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
    kolabchangeitemstagstask.cpp
    updatemessagejob.cpp
    ../imap/settingspasswordrequester.h
    kolabretrievecollectionstask.h
    kolabresource.h
    kolabresourcestate.h
    kolabhelpers.h
    kolabmessagehelper.h
    kolabaddtagtask.h
    kolabchangetagtask.h
    kolabremovetagtask.h
    kolabretrievetagstask.h
    kolabsettings.h
    tagchangehelper.h
    kolabrelationresourcetask.h
    kolabchangeitemstagstask.h
    updatemessagejob.h
)

add_executable(akonadi_kolab_resource ${kolabresource_SRCS} ${kolabresource_common_SRCS})

target_link_libraries(akonadi_kolab_resource
    KPim6::AkonadiCore
    KPim6::IMAP
    KPim6::MailTransport
    KPim6::Mime
    KPim6::AkonadiMime
    KPim6::AkonadiCalendar
    KPim6::IdentityManagementCore
    imapresource
    folderarchivesettings
    pimkolab
    KF6::Contacts
    KF6::CalendarCore
    akonadi-singlefileresource
    KF6::I18n
)

install(FILES kolabresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.akonadi_kolab_resource.desktop DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
    #add_subdirectory(autotests)
endif()

################################## Config plugin ##############################
kcoreaddons_add_plugin(kolabconfig
    INSTALL_NAMESPACE "pim6/akonadi/config"
)

ki18n_wrap_ui(kolabconfig ../imap/setupserverview_desktop.ui)

target_sources(kolabconfig PRIVATE
    ../imap/serverinfodialog.cpp
    ../imap/setupserver.cpp
    ../imap/subscriptiondialog.cpp

    ../imap/serverinfodialog.h
    ../imap/setupserver.h
    ../imap/subscriptiondialog.h
    ${kolabresource_common_SRCS}
)

target_link_libraries(kolabconfig PUBLIC
    KF6::Completion
    KF6::WidgetsAddons
    KF6::I18n
    KF6::Notifications
    KF6::Wallet
    KPim6::AkonadiCore
    KPim6::IMAP
    KPim6::MailTransport
    KPim6::AkonadiWidgets
    KPim6::AkonadiAgentWidgetBase
    KPim6::AkonadiMime
    KPim6::IdentityManagementCore
    KPim6::IdentityManagementWidgets
    KF6::KIOWidgets # SslUi
    qt6keychain
    imapresource
    folderarchivesettings
)

if(HAVE_ACTIVITY_SUPPORT)
    target_link_libraries(kolabconfig PUBLIC KPim6::PimCommonActivities)
endif()
