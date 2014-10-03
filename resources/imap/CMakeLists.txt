include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

option(IMAPRESOURCE_NO_SOLID "Disable the use of solid in the imap resource" FALSE)
if(IMAPRESOURCE_NO_SOLID)
  add_definitions( -DIMAPRESOURCE_NO_SOLID=1 )
endif()

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
  timestampattribute.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  settings.cpp
  subscriptiondialog.cpp
  imapidlemanager.cpp
  resourcestate.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
kde4_add_kcfg_files(imapresource_LIB_SRCS settingsbase.kcfgc)

qt4_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )



qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresourcebase.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt4_add_dbus_adaptor( imapresource_LIB_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresourcebase.h ImapResourceBase
     )

kde4_add_library(imapresource STATIC ${imapresource_LIB_SRCS})

target_link_libraries(imapresource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS})

if( NOT IMAPRESOURCE_NO_SOLID)
  target_link_libraries(imapresource ${KDE4_SOLID_LIBS})
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
kde4_add_ui_files(akonadi_imap_resource_SRCS setupserverview_mobile.ui)
else ()
kde4_add_ui_files(akonadi_imap_resource_SRCS setupserverview_desktop.ui)
endif ()
kde4_add_ui_files(akonadi_imap_resource_SRCS serverinfo.ui)



if (RUNTIME_PLUGINS_STATIC)
    add_definitions(-DMAIL_SERIALIZER_PLUGIN_STATIC)
endif ()

kde4_add_executable(akonadi_imap_resource ${akonadi_imap_resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif ()

target_link_libraries(akonadi_imap_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} imapresource folderarchivesettings)

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_imap_resource akonadi_serializer_mail)
endif ()

if( NOT IMAPRESOURCE_NO_SOLID)
target_link_libraries(akonadi_imap_resource ${KDE4_SOLID_LIBS})
endif()

install(TARGETS akonadi_imap_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )
add_subdirectory( tests )
