include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( imapresource_SRCS
  setupserver.cpp
  imapaccount.cpp
  imapresource.cpp
  settings.cpp
  uidvalidityattribute.cpp
  uidnextattribute.cpp
  noselectattribute.cpp
  subscriptiondialog.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
  ${AKONADI_IMAPATTRIBUTES_SHARED_SOURCES}
)

install( FILES imapresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(imapresource_SRCS setupserverview.ui)
kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.kcfg org.kde.Akonadi.Imap.Settings )
qt4_add_dbus_adaptor( imapresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Settings.xml settings.h Settings
     )
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/imapresource.h org.kde.Akonadi.Imap.Resource.xml )
qt4_add_dbus_adaptor( imapresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Imap.Resource.xml
        imapresource.h ImapResource
     )
kde4_add_kcfg_files(imapresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_imap_resource RUN_UNINSTALLED ${imapresource_SRCS})

target_link_libraries(akonadi_imap_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDEPIMLIBS_KIMAP_LIBS} ${KDEPIMLIBS_MAILTRANSPORT_LIBS} ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} kdepim)

install(TARGETS akonadi_imap_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

