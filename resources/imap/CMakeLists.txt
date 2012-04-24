include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

option(IMAPRESOURCE_NO_SOLID "Disable the use of solid in the imap resource" FALSE)
if(IMAPRESOURCE_NO_SOLID)
  add_definitions( -DIMAPRESOURCE_NO_SOLID=1 )
endif(IMAPRESOURCE_NO_SOLID)

########### next target ###############

set( imapresource_LIB_SRCS
  addcollectiontask.cpp
  additemtask.cpp
  changecollectiontask.cpp
  changeitemtask.cpp
  expungecollectiontask.cpp
  imapaccount.cpp
  imapflags.cpp
  movecollectiontask.cpp
  moveitemtask.cpp
  noselectattribute.cpp
  noinferiorsattribute.cpp
  passwordrequesterinterface.cpp
  removecollectiontask.cpp
  removecollectionrecursivetask.cpp
  removeitemtask.cpp
  resourcestateinterface.cpp
  resourcetask.cpp
  retrievecollectionmetadatatask.cpp
  retrievecollectionstask.cpp
  retrieveitemtask.cpp
  retrieveitemstask.cpp
  sessionpool.cpp
  timestampattribute.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

kde4_add_library(imapresource STATIC ${imapresource_LIB_SRCS})

target_link_libraries(imapresource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} ${KDEPIMLIBS_KPIMUTILS_LIBS})

if( NOT IMAPRESOURCE_NO_SOLID)
  target_link_libraries(imapresource ${KDE4_SOLID_LIBS})
endif( NOT IMAPRESOURCE_NO_SOLID)
########### next target ###############

set( akonadi_imap_resource_SRCS
  imapidlemanager.cpp
  imapresource.cpp
  resourcestate.cpp
  settings.cpp
  settingspasswordrequester.cpp
  setupserver.cpp
  subscriptiondialog.cpp
)

install( FILES imapresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

if (KDEPIM_MOBILE_UI)
kde4_add_ui_files(akonadi_imap_resource_SRCS setupserverview_mobile.ui)
else (KDEPIM_MOBILE_UI)
kde4_add_ui_files(akonadi_imap_resource_SRCS setupserverview_desktop.ui)
endif (KDEPIM_MOBILE_UI)
kde4_add_ui_files(akonadi_imap_resource_SRCS serverinfo.ui)

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
qt4_add_dbus_adaptor( akonadi_imap_resource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt4_add_dbus_adaptor( akonadi_imap_resource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresource.h ImapResource
     )
kde4_add_kcfg_files(akonadi_imap_resource_SRCS settingsbase.kcfgc)

if (RUNTIME_PLUGINS_STATIC)
    add_definitions(-DMAIL_SERIALIZER_PLUGIN_STATIC)
endif (RUNTIME_PLUGINS_STATIC)

kde4_add_executable(akonadi_imap_resource ${akonadi_imap_resource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_imap_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} imapresource)

if (RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_imap_resource akonadi_serializer_mail)
endif (RUNTIME_PLUGINS_STATIC)

if( NOT IMAPRESOURCE_NO_SOLID)
target_link_libraries(akonadi_imap_resource ${KDE4_SOLID_LIBS})
endif( NOT IMAPRESOURCE_NO_SOLID)

install(TARGETS akonadi_imap_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )
if(NOT WINCE)
add_subdirectory( tests )
endif(NOT WINCE)
