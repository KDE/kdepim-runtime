include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${kdepim-runtime_SOURCE_DIR}/resources/shared
    ${kdepim-runtime_SOURCE_DIR}/resources/shared/filestore
    ${kdepim-runtime_SOURCE_DIR}/resources/maildir
    ${kdepim-runtime_SOURCE_DIR}/resources/mbox
    ${CMAKE_CURRENT_SOURCE_DIR}/kmindexreader
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

add_subdirectory(kmindexreader)

########### next target ###############

set( mixedmaildirresource_SRCS
  configdialog.cpp
  mixedmaildirresource.cpp
  mixedmaildirstore.cpp
)

install( FILES mixedmaildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(mixedmaildirresource_SRCS settings.ui)

kde4_add_kcfg_files(mixedmaildirresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mixedmaildirresource.kcfg org.kde.Akonadi.MixedMaildir.Settings)

qt4_add_dbus_adaptor(mixedmaildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_mixedmaildir_resource ${mixedmaildirresource_SRCS})

target_link_libraries(akonadi_mixedmaildir_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  maildir
  mbox
  akonadi-filestore
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
)

install(TARGETS akonadi_mixedmaildir_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MixedMaildir.Settings.xml
        DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
