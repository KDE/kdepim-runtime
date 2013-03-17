project(tinytinyrssresource)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set(tinytinyrssresource_SRCS
  configdialog.cpp tinytinyrssresource.cpp jobs.cpp
)

kde4_add_ui_files(tinytinyrssresource_SRCS
  configdialog.ui
)

kde4_add_kcfg_files(tinytinyrssresource_SRCS settings.kcfgc)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.kcfg
  org.kde.TinyTiny.Rss.Settings
)

qt4_add_dbus_adaptor(tinytinyrssresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.TinyTiny.Rss.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_tinytinyrss_resource RUN_UNINSTALLED ${tinytinyrssresource_SRCS})

if(Q_WS_MAC)
  set_target_properties(akonadi_tinytinyrss_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../../Info.plist.template
  )
  set_target_properties(akonadi_tinytinyrss_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.TinyTiny.rss"
  )
  set_target_properties(akonadi_tinytinyrss_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE TinyTinyRSS Resource"
  )
endif()

include_directories(
    ${QJSON_INCLUDE_DIR}
    ${qjson_INCLUDE_DIR}
)

target_link_libraries(akonadi_tinytinyrss_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KRSS_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEWEBKIT_LIBS}
  ${QJSON_LIBRARIES}
)

install(TARGETS akonadi_tinytinyrss_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
  FILES tinytinyrssresource.desktop
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
)
