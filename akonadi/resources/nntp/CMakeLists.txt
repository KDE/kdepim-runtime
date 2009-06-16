include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( nntpresource_SRCS
  nntpcollectionattribute.cpp
  nntpresource.cpp
  configdialog.cpp
  settings.cpp
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

install( FILES nntpresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/nntpresource.kcfg org.kde.Akonadi.NNTP.Settings )

qt4_add_dbus_adaptor( nntpresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.NNTP.Settings.xml settings.h Settings
)
kde4_add_ui_files(nntpresource_SRCS configdialog.ui)
kde4_add_kcfg_files(nntpresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_nntp_resource RUN_UNINSTALLED ${nntpresource_SRCS})

target_link_libraries(akonadi_nntp_resource ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTCORE_LIBRARY})

install(TARGETS akonadi_nntp_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
