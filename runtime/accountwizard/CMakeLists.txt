set(accountwizard_srcs
  main.cpp
  dialog.cpp
  providerpage.cpp
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
  servertest.cpp
  personaldatapage.cpp
  ispdb/ispdb.cpp
)

kde4_add_ui_files(accountwizard_srcs
  providerpage.ui
  typepage.ui
  loadpage.ui
  setuppage.ui
  personaldatapage.ui
)

kde4_add_executable(accountwizard ${accountwizard_srcs})

target_link_libraries(accountwizard
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDE4_KROSSCORE_LIBS}
  ${KDE4_KROSSUI_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KNEWSTUFF3_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBRARY}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTUITOOLS_LIBRARY}
)

install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES accountwizard.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES accountwizard.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES accountwizard-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
