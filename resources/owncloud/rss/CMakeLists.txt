project(owncloudrssresource)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(owncloudrssresource_SRCS
  configdialog.cpp owncloudrssresource.cpp
)

kde4_add_ui_files(owncloudrssresource_SRCS
  configdialog.ui
)

kde4_add_kcfg_files(owncloudrssresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg
  org.kde.Owncloud.Rss.Settings
)

qt4_add_dbus_adaptor(owncloudrssresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Owncloud.Rss.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_owncloudrss_resource RUN_UNINSTALLED ${owncloudrssresource_SRCS})

if(Q_WS_MAC)
  set_target_properties(akonadi_owncloudrss_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
  )
  set_target_properties(akonadi_owncloudrss_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Owncloud.rss"
  )
  set_target_properties(akonadi_owncloudrss_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Owncloud RSS Resource"
  )
endif()

target_link_libraries(akonadi_owncloudrss_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KRSS_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEWEBKIT_LIBS}
  ${QJSON_LIBRARIES}
  ${QJSON_LIBRARY} # for Mac OSX
)

install(TARGETS akonadi_owncloudrss_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES owncloudrssresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
