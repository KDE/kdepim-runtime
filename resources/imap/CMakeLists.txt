include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( libimapresource_SRCS
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
  passwordrequesterinterface.cpp
  removecollectiontask.cpp
  removeitemtask.cpp
  resourcestateinterface.cpp
  resourcetask.cpp
  retrievecollectionmetadatatask.cpp
  retrievecollectionstask.cpp
  retrieveitemtask.cpp
  retrieveitemstask.cpp
  sessionpool.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

kde4_add_library(libimapresource STATIC ${libimapresource_SRCS})

target_link_libraries(libimapresource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} kdepim-copy)


########### next target ###############

set( imapresource_SRCS
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
kde4_add_ui_files(imapresource_SRCS setupserverview_mobile.ui)
else (KDEPIM_MOBILE_UI)
kde4_add_ui_files(imapresource_SRCS setupserverview_desktop.ui)
endif (KDEPIM_MOBILE_UI)
kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
qt4_add_dbus_adaptor( imapresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.h org.kde.Akonadi.Imap.Resource.xml OPTIONS -a )
qt4_add_dbus_adaptor( imapresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresource.h ImapResource
     )
kde4_add_kcfg_files(imapresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_imap_resource ${imapresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Imap")
  set_target_properties(akonadi_imap_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi IMAP Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_imap_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS} kdepim-copy libimapresource)

install(TARGETS akonadi_imap_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory( wizard )
add_subdirectory( tests )
