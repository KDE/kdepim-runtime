set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(contactsresource_SRCS
  contactsresource.cpp
  settings.cpp
  settingsdialog.cpp
)

kde4_add_ui_files(contactsresource_SRCS ui/settingsdialog.ui ui/groupeditor.ui)

kde4_add_kcfg_files(contactsresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.GoogleContacts.Settings
)

qt4_add_dbus_adaptor(contactsresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.GoogleContacts.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)

kde4_add_executable(akonadi_googlecontacts_resource RUN_UNINSTALLED ${contactsresource_SRCS})

target_link_libraries(akonadi_googlecontacts_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${LIBKGOOGLE_LIBRARY}
)

install(TARGETS akonadi_googlecontacts_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googlecontactsresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
