add_subdirectory(saslplugin)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )



include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${LibKGAPI2_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${QJSON_INCLUDE_DIR} 
    ${qjson_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../folderarchivesettings/
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set(gmailresource_SRCS
    gmailresource.cpp
    gmailretrievecollectionstask.cpp
    gmailretrieveitemstask.cpp
    gmailmessagehelper.cpp
    gmailpasswordrequester.cpp
    gmailresourcestate.cpp
    gmailconfigdialog.cpp
    gmailsettings.cpp
    gmaillinkitemstask.cpp
    gmaillabelattribute.cpp
    gmailchangeitemslabelstask.cpp
)

kde4_add_ui_files(gmailresource_SRCS gmailconfigdialog.ui)
#kde4_add_kcfg_files(gmailresource_SRCS settingsbase.kcfgc)

kde4_add_executable(akonadi_gmail_resource ${gmailresource_SRCS})
target_link_libraries(akonadi_gmail_resource
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${KDEPIMLIBS_KIMAP_LIBS}
    ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
    ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
    ${LibKGAPI2_LIBRARY}
    ${QJSON_LIBRARIES}
    imapresource
    folderarchivesettings
)

install(FILES gmailresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(TARGETS akonadi_gmail_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
