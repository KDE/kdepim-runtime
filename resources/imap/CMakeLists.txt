kde_enable_exceptions()

option(IMAPRESOURCE_NO_SOLID "Disable the use of solid in the imap resource" FALSE)
if(IMAPRESOURCE_NO_SOLID)
  add_definitions( -DIMAPRESOURCE_NO_SOLID=1 )
endif()

find_package(KF5PimIdentities ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransport ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Imap ${KF5_VERSION} CONFIG REQUIRED)

########### next target ###############

set( imapresource_LIB_SRCS
  addcollectiontask.cpp
  additemtask.cpp
  changecollectiontask.cpp
  changeitemsflagstask.cpp
  changeitemtask.cpp
  expungecollectiontask.cpp
  highestmodseqattribute.cpp
  imapaccount.cpp
  imapflags.cpp
  movecollectiontask.cpp
  moveitemstask.cpp
  noselectattribute.cpp
  noinferiorsattribute.cpp
  passwordrequesterinterface.cpp
  removecollectionrecursivetask.cpp
  removeitemstask.cpp
  resourcestateinterface.cpp
  resourcetask.cpp
  retrievecollectionmetadatatask.cpp
  retrievecollectionstask.cpp
  retrieveitemtask.cpp
  retrieveitemstask.cpp
  searchtask.cpp
  sessionpool.cpp
  timestampattribute.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

add_library(imapresource STATIC ${imapresource_LIB_SRCS})

target_link_libraries(imapresource KF5::AkonadiCore KF5::Imap KF5::MailTransport KF5::KIOCore KF5::KMime KF5::AkonadiKMime KF5::PimIdentities KF5::AkonadiAgentBase)

if( NOT IMAPRESOURCE_NO_SOLID)
  target_link_libraries(imapresource ${KDE4_SOLID_LIBS})
endif()
########### next target ###############

set( akonadi_imap_resource_SRCS
  main.cpp
  imapidlemanager.cpp
  imapresource.cpp
  resourcestate.cpp
  settings.cpp
  settingspasswordrequester.cpp
  setupserver.cpp
  serverinfodialog.cpp
  subscriptiondialog.cpp
)

install( FILES imapresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

if (KDEPIM_MOBILE_UI)
qt5_wrap_ui(akonadi_imap_resource_SRCS setupserverview_mobile.ui)
else ()
qt5_wrap_ui(akonadi_imap_resource_SRCS setupserverview_desktop.ui)
endif ()
qt5_wrap_ui(akonadi_imap_resource_SRCS serverinfo.ui)

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
qt5_add_dbus_adaptor( akonadi_imap_resource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt5_add_dbus_adaptor( akonadi_imap_resource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresource.h ImapResource
     )
kconfig_add_kcfg_files(akonadi_imap_resource_SRCS settingsbase.kcfgc)

if (RUNTIME_PLUGINS_STATIC)
    add_definitions(-DMAIL_SERIALIZER_PLUGIN_STATIC)
endif ()

add_executable(akonadi_imap_resource ${akonadi_imap_resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif ()

target_link_libraries(akonadi_imap_resource KF5::AkonadiCore     KF5::Imap KF5::MailTransport KF5::KIOCore KF5::KMime KF5::AkonadiKMime KF5::KF5PimIdentities imapresource folderarchivesettings)

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_imap_resource akonadi_serializer_mail)
endif ()

if( NOT IMAPRESOURCE_NO_SOLID)
target_link_libraries(akonadi_imap_resource ${KDE4_SOLID_LIBS})
endif()

install(TARGETS akonadi_imap_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )
add_subdirectory( tests )
