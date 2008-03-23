INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
${QT_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR} )

SET(tray_sources main.cpp dock.cpp)

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/dock.h org.kde.Akonadi.Tray.xml )
qt4_add_dbus_adaptor( tray_sources org.kde.Akonadi.Tray.xml dock.h Dock::Dock )

KDE4_ADD_EXECUTABLE(akonaditray ${tray_sources})
TARGET_LINK_LIBRARIES(akonaditray ${KDE4_KDEUI_LIBRARY} ${KDE4_KNOTIFYCONFIG_LIBS} ${AKONADI_LIBS})

INSTALL(TARGETS akonaditray DESTINATION ${BIN_INSTALL_DIR})
#INSTALL(FILES  akonaditray.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
# INSTALL( FILES akonaditray.notifyrc DESTINATION ${DATA_INSTALL_DIR}/akonaditray  )
# INSTALL( FILES org.kde.Akonadi.Tray.xml DESTINATION ${KDE4_DBUS_INTERFACES_DIR} )
