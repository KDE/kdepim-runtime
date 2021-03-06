add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")

set(WITH_GMAIL_XOAUTH2 TRUE)

configure_file(config-imap.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-imap.h)

########### next target ###############

set( imapresource_LIB_SRCS
  addcollectiontask.cpp
  additemtask.cpp
  batchfetcher.cpp
  changecollectiontask.cpp
  changeitemsflagstask.cpp
  changeitemtask.cpp
  expungecollectiontask.cpp
  highestmodseqattribute.cpp
  imapaccount.cpp
  imapflags.cpp
  imapresourcebase.cpp
  messagehelper.cpp
  movecollectiontask.cpp
  moveitemstask.cpp
  noselectattribute.cpp
  noinferiorsattribute.cpp
  passwordrequesterinterface.cpp
  removecollectionrecursivetask.cpp
  resourcestateinterface.cpp
  resourcetask.cpp
  retrievecollectionmetadatatask.cpp
  retrievecollectionstask.cpp
  retrieveitemtask.cpp
  retrieveitemstask.cpp
  searchtask.cpp
  sessionpool.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  settings.cpp
  subscriptiondialog.cpp
  imapidlemanager.cpp
  resourcestate.cpp
  collectionmetadatahelper.cpp
  replacemessagejob.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

ecm_qt_declare_logging_category(imapresource_LIB_SRCS HEADER imapresource_debug.h IDENTIFIER IMAPRESOURCE_LOG CATEGORY_NAME org.kde.pim.imapresource
        DESCRIPTION "imap resource (kdepim-runtime)"
        OLD_CATEGORY_NAMES log_imapresource
        EXPORT KDEPIMRUNTIME
    )
ecm_qt_declare_logging_category(imapresource_LIB_SRCS HEADER imapresource_trace.h
        IDENTIFIER IMAPRESOURCE_TRACE
        CATEGORY_NAME org.kde.pim.imapresource.trace
        DESCRIPTION "resource kolab trace (kdepim-runtime)"
        EXPORT KDEPIMRUNTIME
        )

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
kconfig_add_kcfg_files(imapresource_LIB_SRCS settingsbase.kcfgc)

qt_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )

qt_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresourcebase.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresourcebase.h ImapResourceBase
     )

add_library(imapresource STATIC ${imapresource_LIB_SRCS})

target_link_libraries(imapresource
    Qt::DBus
    Qt::Network
    KF5::AkonadiCore
    KF5::IMAP
    KF5::MailTransport
    KF5::KIOWidgets
    KF5::Mime
    KF5::AkonadiMime
    KF5::IdentityManagement
    KF5::AkonadiAgentBase
    KF5::I18n
    KF5::WindowSystem
    akonadi-singlefileresource
    KF5::PimCommon
    KF5::Libkdepim
    qt5keychain
)

########### next target ###############

add_executable(akonadi_imap_resource)
ki18n_wrap_ui(akonadi_imap_resource setupserverview_desktop.ui)
target_sources( akonadi_imap_resource PRIVATE
  main.cpp
  imapresource.cpp
  resourcestate.cpp
  settingspasswordrequester.cpp
  setupserver.cpp
  serverinfodialog.cpp
  passwordrequester.cpp
  gmailpasswordrequester.cpp
)

install( FILES imapresource.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents" )



if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_imap_resource PROPERTIES UNITY_BUILD ON)
endif()
if( APPLE )
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif ()

target_link_libraries(akonadi_imap_resource
    Qt::DBus
    KF5::AkonadiCore
    KF5::IMAP
    akonadi-singlefileresource
    KF5::AkonadiWidgets
    KF5::MailTransport
    KF5::Mime
    KF5::AkonadiMime
    KF5::IdentityManagement
    KF5::I18n
    imapresource
    KF5::TextWidgets
    folderarchivesettings
    KPim::GAPICore
)

install(TARGETS akonadi_imap_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )

if(BUILD_TESTING)
    add_subdirectory( autotests )
    add_subdirectory( tests )
endif()
