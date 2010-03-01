set(accountwizard_srcs
  main.cpp
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
)

kde4_add_ui_files(accountwizard_srcs
  typepage.ui
  loadpage.ui
  setuppage.ui
)
kde4_add_executable(accountwizard ${accountwizard_srcs})

target_link_libraries(accountwizard
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDE4_KROSSCORE_LIBS}
  ${KDE4_KROSSUI_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTUITOOLS_LIBRARY}
)

install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})

