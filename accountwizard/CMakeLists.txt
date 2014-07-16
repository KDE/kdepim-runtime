add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

# allow to disable GHNS support on resource-constrained systems
option( ACCOUNTWIZARD_NO_GHNS "Disable GHNS support in the account wizard" ${KDEPIM_MOBILE_UI} )
if ( ACCOUNTWIZARD_NO_GHNS )
  add_definitions( -DACCOUNTWIZARD_NO_GHNS )
endif ()

set(accountwizard_srcs
  dialog.cpp
  typepage.cpp
  loadpage.cpp
  global.cpp
  page.cpp
  dynamicpage.cpp
  setupmanager.cpp
  setuppage.cpp
  resource.cpp
  setupobject.cpp
  transport.cpp
  configfile.cpp
  ldap.cpp
  identity.cpp
  setupispdb.cpp
  setupautoconfigkolabmail.cpp
  setupautoconfigkolabldap.cpp
  setupautoconfigkolabfreebusy.cpp
  servertest.cpp
  personaldatapage.cpp
  ispdb/ispdb.cpp
  ispdb/autoconfigkolabmail.cpp
  ispdb/autoconfigkolabldap.cpp
  ispdb/autoconfigkolabfreebusy.cpp
)

kde4_add_ui_files(accountwizard_srcs
  ui/typepage.ui
  ui/loadpage.ui
  ui/setuppage.ui
  ui/personaldatapage.ui
)

set(accountwizard_libs
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDE4_KROSSCORE_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTUITOOLS_LIBRARY}
)

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  set(accountwizard_srcs ${accountwizard_srcs} providerpage.cpp)
  kde4_add_ui_files(accountwizard_srcs ui/providerpage.ui)
  set(accountwizard_libs ${accountwizard_libs} ${KDE4_KNEWSTUFF3_LIBS})
endif ()

kde4_add_executable(accountwizard ${accountwizard_srcs} main.cpp)
target_link_libraries(accountwizard ${accountwizard_libs})

kde4_add_plugin(accountwizard_plugin ${accountwizard_srcs} inprocess-main.cpp)
target_link_libraries(accountwizard_plugin ${accountwizard_libs})

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  install(FILES accountwizard.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
endif ()
install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS accountwizard_plugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(PROGRAMS accountwizard.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES accountwizard-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

add_subdirectory(wizards)
add_subdirectory(ispdb)