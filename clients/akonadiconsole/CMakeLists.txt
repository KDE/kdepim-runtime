include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

if(NEPOMUK_FOUND)
  set(AKONADICONSOLE_EXTRA_LIBS ${NEPOMUK_LIBRARIES})
  include_directories(${NEPOMUK_INCLUDES})
endif(NEPOMUK_FOUND)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(akonadiconsole_bin_SRCS
  agentwidget.cpp
  browserwidget.cpp
  collectionattributespage.cpp
  collectioninternalspage.cpp
  collectionaclpage.cpp
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
# Use intalled interface once we can depend on Akoandi 1.1
set_source_files_properties( org.freedesktop.Akonadi.DebugInterface.xml PROPERTIES CLASSNAME DebugInterface )
qt4_add_dbus_interface( akonadiconsole_bin_SRCS
  org.freedesktop.Akonadi.DebugInterface.xml
  debuginterface
)

kde4_add_ui_files(akonadiconsole_bin_SRCS
  browserwidget_itemview.ui
  browserwidget_contentview.ui
  collectionattributespage.ui
  collectioninternalspage.ui
  collectionaclpage.ui
)

kde4_add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries( akonadiconsole_bin
  ${KDE4_AKONADI_LIBS}
  akonadi-kabccommon
  ${KDE4_AKONADI_KMIME_LIBS}
  akonadi-kcal
  kdepim
  ${KDE4_KDEUI_LIBS}
  ${AKONADICONSOLE_EXTRA_LIBS}
)

install( TARGETS akonadiconsole_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES akonadiconsole.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES akonadiconsoleui.rc DESTINATION ${DATA_INSTALL_DIR}/akonadiconsole )
