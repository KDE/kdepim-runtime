#
#  SPDX-FileCopyrightText: 2015-2018 Krzysztof Nowicki <krissn@op.pl>
#
#  SPDX-License-Identifier: LGPL-2.0-or-later

# Build a separate resource for sending e-mail. This is needed for KMail which assumes that
# a single resource is not able to both send and receive e-mail.
set(HAVE_SEPARATE_MTA_RESOURCE TRUE)

set(AKONADI_EWS_VERSION ${KDEPIM_RUNTIME_VERSION})

add_subdirectory(ewsclient)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ewsconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ewsconfig.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ewsclient)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/ewsclient)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_ews_resource\")

set(ewsresource_SRCS
    abchperson/ewsabchpersonhandler.cpp
    abchperson/ewscreateabchpersonjob.cpp
    abchperson/ewsfetchabchpersondetailjob.cpp
    abchperson/ewsmodifyabchpersonjob.cpp
    calendar/ewscalendarhandler.cpp
    calendar/ewscreatecalendarjob.cpp
    calendar/ewsfetchcalendardetailjob.cpp
    calendar/ewsmodifycalendarjob.cpp
    contact/ewscontacthandler.cpp
    contact/ewscreatecontactjob.cpp
    contact/ewsfetchcontactdetailjob.cpp
    contact/ewsmodifycontactjob.cpp
    mail/ewscreatemailjob.cpp
    mail/ewsfetchmaildetailjob.cpp
    mail/ewsmailhandler.cpp
    mail/ewsmodifymailjob.cpp
    tags/ewsakonaditagssyncjob.cpp
    tags/ewsglobaltagsreadjob.cpp
    tags/ewsglobaltagswritejob.cpp
    tags/ewstagstore.cpp
    tags/ewsupdateitemstagsjob.cpp
    task/ewscreatetaskjob.cpp
    task/ewsfetchtaskdetailjob.cpp
    task/ewsmodifytaskjob.cpp
    task/ewstaskhandler.cpp
    ewsautodiscoveryjob.cpp
    ewsconfigdialog.cpp
    ewscreateitemjob.cpp
    ewsfetchfoldersjob.cpp
    ewsfetchfoldersincrjob.cpp
    ewsfetchitemsjob.cpp
    ewsfetchitemdetailjob.cpp
    ewsitemhandler.cpp
    ewsmodifyitemjob.cpp
    ewsmodifyitemflagsjob.cpp
    ewsprogressdialog.cpp
    ewsresource.cpp
    ewssettings.cpp
    ewssubscribedfoldersjob.cpp
    ewssubscriptionmanager.cpp
    ewssubscriptionwidget.cpp)

ecm_qt_declare_logging_category(ewsresource_SRCS
    HEADER ewsres_debug.h
    IDENTIFIER EWSRES_LOG
    CATEGORY_NAME org.kde.pim.ews
    DESCRIPTION "ews resource (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


ecm_qt_declare_logging_category(ewsresource_SRCS
    HEADER ewsres_agentif_debug.h
    IDENTIFIER EWSRES_AGENTIF_LOG
    CATEGORY_NAME org.kde.pim.ews.agentif
    DESCRIPTION "exchange ews agent (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
    )


ki18n_wrap_ui(ewsresource_SRCS ewsconfigdialog.ui)

kconfig_add_kcfg_files(ewsresource_SRCS ewssettingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ewsresource.kcfg org.kde.Akonadi.Ews.Settings)
qt5_add_dbus_adaptor(ewsresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Settings.xml ewssettings.h EwsSettings ewssettingsadaptor EwsSettingsAdaptor
    )

qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/ewsresource.h org.kde.Akonadi.Ews.Resource.xml OPTIONS -a )
qt5_add_dbus_adaptor(ewsresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Resource.xml ewsresource.h EwsResource ewsresourceadaptor EwsResourceAdaptor
    )
qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/ewssettings.h org.kde.Akonadi.Ews.Wallet.xml OPTIONS -a )
qt5_add_dbus_adaptor(ewsresource_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Wallet.xml ewssettings.h EwsSettings ewswalletadaptor EwsWalletAdaptor
    )



add_executable(akonadi_ews_resource ${ewsresource_SRCS})
target_link_libraries(akonadi_ews_resource
    KF5::AkonadiAgentBase
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::KIOCore
    KF5::Mime
    KF5::CalendarCore
    KF5::Contacts
    KF5::Wallet
    KF5::WidgetsAddons
    KF5::I18n
    KF5::WindowSystem
    KF5::Notifications
    KF5::ConfigWidgets
    ewsclient)

if (HAVE_SEPARATE_MTA_RESOURCE)
    set(ewsmtaresource_SRCS
        ewsmtaconfigdialog.cpp
        ewsmtaresource.cpp)
    ecm_qt_declare_logging_category(ewsmtaresource_SRCS
        HEADER ewsres_mta_debug.h
        IDENTIFIER EWSRES_MTA_LOG
        CATEGORY_NAME org.kde.pim.ews.mta
        DESCRIPTION "ews mta (kdepim-runtime)"
        EXPORT KDEPIMRUNTIME
        )


    ki18n_wrap_ui(ewsmtaresource_SRCS ewsmtaconfigdialog.ui)

    kconfig_add_kcfg_files(ewsmtaresource_SRCS ewsmtasettings.kcfgc)
    kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ewsmtaresource.kcfg org.kde.Akonadi.EwsMta.Settings)
    qt5_add_dbus_adaptor(ewsmtaresource_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.EwsMta.Settings.xml ewsmtasettings.h EwsMtaSettings ewsmtasettingsadaptor
        )
    qt5_add_dbus_interface(ewsmtaresource_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Ews.Resource.xml ewsresourceinterface)

    add_executable(akonadi_ewsmta_resource ${ewsmtaresource_SRCS})
    target_link_libraries(akonadi_ewsmta_resource
        KF5::AkonadiAgentBase
        KF5::AkonadiCore
        KF5::AkonadiWidgets
        KF5::I18n
        KF5::WindowSystem
        KF5::Mime)

    set(EWS_MTA_CAPABILITIES "X-EwsMailTransport")
else ()
    set(EWS_MTA_CAPABILITIES "MailTransport,Synchronizable")
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ewsresource.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/ewsresource.desktop)

if (BUILD_TESTING)
    add_subdirectory(test)
endif ()


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ewsresource.desktop
    DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )
install(TARGETS akonadi_ews_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

if (HAVE_SEPARATE_MTA_RESOURCE)
    install(FILES ewsmtaresource.desktop
        DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )
    install(TARGETS akonadi_ewsmta_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
endif ()

ecm_install_icons(
    ICONS
    icons/16-apps-akonadi-ews.png
    icons/22-apps-akonadi-ews.png
    icons/24-apps-akonadi-ews.png
    icons/32-apps-akonadi-ews.png
    icons/48-apps-akonadi-ews.png
    icons/64-apps-akonadi-ews.png
    icons/72-apps-akonadi-ews.png
    icons/96-apps-akonadi-ews.png
    icons/128-apps-akonadi-ews.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
    )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/akonadi_ews_resource.notifyrc
    DESTINATION "${KNOTIFYRC_INSTALL_DIR}")
