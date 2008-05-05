include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

set(akonadiconsole_bin_SRCS
  agentwidget.cpp
  browserwidget.cpp
  collectionattributespage.cpp
  collectioninternalspage.cpp
  connectionpage.cpp
  debugwidget.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
  searchdialog.cpp
)

qt4_add_dbus_interfaces( akonadiconsole_bin_SRCS
  ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.TracerNotification.xml
)

kde4_add_ui_files(akonadiconsole_bin_SRCS
  browserwidget_itemview.ui
  browserwidget_contentview.ui
  collectionattributespage.ui
  collectioninternalspage.ui
)

kde4_add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries( akonadiconsole_bin ${KDE4_AKONADI_LIBS} akonadi-kabc ${KDE4_AKONADI_KMIME_LIBS} akonadi-kcal kdepim ${KDE4_KDEUI_LIBRARY} )

install( TARGETS akonadiconsole_bin DESTINATION ${BIN_INSTALL_DIR} )
install( FILES akonadiconsole.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
INSTALL( FILES akonadiconsoleui.rc DESTINATION ${DATA_INSTALL_DIR}/akonadiconsole )
