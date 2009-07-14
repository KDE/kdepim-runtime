project(akonadiconsole)

include_directories(${AKONADI_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIR})
include_directories( BEFORE ${CMAKE_CURRENT_BINARY_DIR} )

if(NEPOMUK_FOUND)
  set(AKONADICONSOLE_EXTRA_LIBS ${NEPOMUK_LIBRARIES})
  include_directories(${NEPOMUK_INCLUDES})
endif(NEPOMUK_FOUND)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
configure_file(config-akonadiconsole.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadiconsole.h)

set(akonadiconsole_bin_SRCS
  agentwidget.cpp
  browserwidget.cpp
  collectionattributespage.cpp
  collectioninternalspage.cpp
  collectionaclpage.cpp
  connectionpage.cpp
  dbaccess.cpp
  dbbrowser.cpp
  dbconsole.cpp
  debugwidget.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
  notificationmodel.cpp
  notificationmonitor.cpp
  rawsocketconsole.cpp
  searchdialog.cpp
  jobtracker.cpp
  jobtrackerwidget.cpp
  jobtrackermodel.cpp
)

qt4_generate_dbus_interface( jobtracker.h org.freedesktop.Akonadi.JobTracker.xml )
qt4_add_dbus_adaptor( akonadiconsole_bin_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Akonadi.JobTracker.xml jobtracker.h JobTracker )

qt4_add_dbus_interfaces( akonadiconsole_bin_SRCS
  ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.TracerNotification.xml
)
# Use intalled interface once we can depend on Akoandi 1.1
set_source_files_properties( org.freedesktop.Akonadi.DebugInterface.xml PROPERTIES CLASSNAME DebugInterface )
qt4_add_dbus_interface( akonadiconsole_bin_SRCS
  org.freedesktop.Akonadi.DebugInterface.xml
  debuginterface
)

set_source_files_properties(${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml
  PROPERTIES INCLUDE "akonadi/private/notificationmessage_p.h")
qt4_add_dbus_interface( akonadiconsole_bin_SRCS
  ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml
  notificationmanagerinterface
)

kde4_add_ui_files(akonadiconsole_bin_SRCS
  agentwidget.ui
  browserwidget_itemview.ui
  browserwidget_contentview.ui
  collectionattributespage.ui
  collectioninternalspage.ui
  collectionaclpage.ui
  dbbrowser.ui
  dbconsole.ui
  rawsocketconsole.ui
)

kde4_add_kcfg_files(akonadiconsole_bin_SRCS settings.kcfgc)

kde4_add_app_icon(akonadiconsole_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/akonadi.png")

kde4_add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries( akonadiconsole_bin
  ${KDEPIMLIBS_AKONADI_LIBS}
  akonadi_next
  akonadi-kabccommon
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  akonadi-kcal
  akonadi-xml
  kdepim-copy
  ${KDE4_KUTILS_LIBS}
  ${KDE4_KIO_LIBS}
  ${AKONADICONSOLE_EXTRA_LIBS}
  ${QT_QTSQL_LIBRARY}
  ${QT_QTNETWORK_LIBRARY}
  ${AKONADI_COMMON_LIBRARIES}
)

install( TARGETS akonadiconsole_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
install( FILES akonadiconsole.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES akonadiconsoleui.rc DESTINATION ${DATA_INSTALL_DIR}/akonadiconsole )
