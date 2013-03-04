set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(readerresource_SRCS
  changeditemscache.cpp
  readerresource.cpp
  settings.cpp
  settingsdialog.cpp
  ../common/googleaccountmanager.cpp
  ../common/googleresource.cpp
  ../common/googlesettings.cpp
  ../common/googlesettingsdialog.cpp
)

kde4_add_kcfg_files(readerresource_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg
  org.kde.Akonadi.GoogleReader.Settings
)

qt4_add_dbus_adaptor(readerresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.GoogleReader.Settings.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.h Settings
)

kde4_add_executable(akonadi_googlereader_resource RUN_UNINSTALLED ${readerresource_SRCS})

target_link_libraries(akonadi_googlereader_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KRSS_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${LibKGAPI2_LIBRARY}
)

install(TARGETS akonadi_googlereader_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES googlereaderresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
