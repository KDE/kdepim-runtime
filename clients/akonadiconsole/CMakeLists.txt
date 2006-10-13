include_directories(${CMAKE_SOURCE_DIR}/akonadi)

set(akonadiconsole_bin_SRCS
  agentwidget.cpp
  collectionwidget.cpp
  connectionpage.cpp
  debugwidget.cpp
  main.cpp
  mainwidget.cpp
  mainwindow.cpp
  profilewidget.cpp
)

qt4_add_dbus_interfaces( akonadiconsole_bin_SRCS
  ${CMAKE_SOURCE_DIR}/akonadi/server/interfaces/org.kde.Akonadi.TracerNotification.xml
)

kde4_automoc(akonadiconsole_bin ${akonadiconsole_bin_SRCS})

kde4_add_executable(akonadiconsole_bin ${akonadiconsole_bin_SRCS})
set_target_properties(akonadiconsole_bin PROPERTIES OUTPUT_NAME akonadiconsole)

target_link_libraries( akonadiconsole_bin akonadicomponents )

install(TARGETS akonadiconsole_bin DESTINATION ${BIN_INSTALL_DIR} )
