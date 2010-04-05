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
  identitypage.cpp
  identity.cpp
  servertest.cpp
)

kde4_add_ui_files(accountwizard_srcs
  providerpage.ui
  typepage.ui
  loadpage.ui
  setuppage.ui
  identitypage.ui
)

kde4_add_executable(accountwizard ${accountwizard_srcs})

target_link_libraries(accountwizard
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDE4_KROSSCORE_LIBS}
  ${KDE4_KROSSUI_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KNEWSTUFF3_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTUITOOLS_LIBRARY}
)

install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES accountwizard.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
