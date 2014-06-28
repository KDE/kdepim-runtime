include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../imap
    ${CMAKE_CURRENT_BINARY_DIR}/../imap
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${Libkolab_INCLUDES}
    ${Libkolabxml_INCLUDES}
)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set(kolabresource_SRCS
    ../imap/imapresource.cpp
    ../imap/settingspasswordrequester.cpp
    ../imap/setupserver.cpp
    ../imap/serverinfodialog.cpp
    kolabretrievecollectionstask.cpp
    kolabresource.cpp
    kolabresourcestate.cpp
    kolabhelpers.cpp
    kolabmessagehelper.cpp
)

kde4_add_kcfg_files(kolabresource_SRCS ../imap/settingsbase.kcfgc)

if (KDEPIM_MOBILE_UI)
kde4_add_ui_files(kolabresource_SRCS ../imap/setupserverview_mobile.ui)
else ()
kde4_add_ui_files(kolabresource_SRCS ../imap/setupserverview_desktop.ui)
endif ()
kde4_add_ui_files(kolabresource_SRCS ../imap/serverinfo.ui)

kde4_add_executable(akonadi_kolab_resource ${kolabresource_SRCS})
target_link_libraries(akonadi_kolab_resource
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
    imapresource
    folderarchivesettings
    ${Libkolab_LIBRARIES}
    ${Libkolabxml_LIBRARIES}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_KCALCORE_LIBS}
)

install(FILES kolabresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
install(TARGETS akonadi_kolab_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
