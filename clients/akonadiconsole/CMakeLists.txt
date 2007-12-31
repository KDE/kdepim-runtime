include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/akonadi/libakonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

set(akonadiconsole_bin_SRCS
  agentwidget.cpp
  browserwidget.cpp
  connectionpage.cpp
  debugwidget.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
  profilewidget.cpp
)

qt4_add_dbus_interfaces( akonadiconsole_bin_SRCS
  ../../server/interfaces/org.kde.Akonadi.TracerNotification.xml
)

kde4_add_ui_files(akonadiconsole_bin_SRCS browserwidget_itemview.ui browserwidget_contentview.ui)

kde4_add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries( akonadiconsole_bin akonadicomponents kabcakonadi kmimeakonadi kdepim ${KDE4_KDEUI_LIBRARY} )

install(TARGETS akonadiconsole_bin DESTINATION ${BIN_INSTALL_DIR} )
