

option(IMAPRESOURCE_NO_SOLID "Disable the use of solid in the imap resource" FALSE)
if(IMAPRESOURCE_NO_SOLID)
  add_definitions( -DIMAPRESOURCE_NO_SOLID=1 )
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_imap_resource\")

########### next target ###############

set( imapresource_LIB_SRCS
  imapresource_debug.cpp
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
  timestampattribute.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  settings.cpp
  subscriptiondialog.cpp
  imapidlemanager.cpp
  resourcestate.cpp
  resource_imap_debug.cpp
)



kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
kconfig_add_kcfg_files(imapresource_LIB_SRCS settingsbase.kcfgc)

qt5_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )

qt5_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresourcebase.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt5_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresourcebase.h ImapResourceBase
     )

add_library(imapresource STATIC ${imapresource_LIB_SRCS})

target_link_libraries(imapresource KF5::AkonadiCore KF5::Imap KF5::MailTransport KF5::KIOCore KF5::Mime KF5::AkonadiMime KF5::IdentityManagement KF5::AkonadiAgentBase KF5::I18n)

if( NOT IMAPRESOURCE_NO_SOLID)
  target_link_libraries(imapresource )
endif()
########### next target ###############

set( akonadi_imap_resource_SRCS
  main.cpp
  imapresource.cpp
  resourcestate.cpp
  settingspasswordrequester.cpp
  setupserver.cpp
  serverinfodialog.cpp
)

install( FILES imapresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

if (KDEPIM_MOBILE_UI)
ki18n_wrap_ui(akonadi_imap_resource_SRCS setupserverview_mobile.ui)
else ()
ki18n_wrap_ui(akonadi_imap_resource_SRCS setupserverview_desktop.ui)
endif ()
ki18n_wrap_ui(akonadi_imap_resource_SRCS serverinfo.ui)

if (RUNTIME_PLUGINS_STATIC)
    add_definitions(-DMAIL_SERIALIZER_PLUGIN_STATIC)
endif ()

add_executable(akonadi_imap_resource ${akonadi_imap_resource_SRCS})

if (Q_OS_OSX)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif ()

target_link_libraries(akonadi_imap_resource KF5::AkonadiCore     KF5::Imap KF5::MailTransport KF5::KIOCore KF5::Mime KF5::AkonadiMime KF5::IdentityManagement imapresource folderarchivesettings KF5::I18n akonadi-singlefileresource)

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_imap_resource akonadi_serializer_mail)
endif ()

if( NOT IMAPRESOURCE_NO_SOLID)
target_link_libraries(akonadi_imap_resource )
endif()

install(TARGETS akonadi_imap_resource ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )
if (BUILD_TESTING)
   add_subdirectory( autotests )
endif()
add_subdirectory( tests )
