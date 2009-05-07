INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
${QT_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR} ${KNOTIFICATIONITEM_INCLUDE_DIR} )

SET(tray_sources main.cpp dock.cpp backup.cpp backupassistant.cpp restore.cpp restoreassistant.cpp global.cpp)

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/dock.h
org.freedesktop.akonaditray.xml )
qt4_add_dbus_adaptor( tray_sources org.freedesktop.akonaditray.xml dock.h Dock )

KDE4_ADD_EXECUTABLE(akonaditray ${tray_sources})
TARGET_LINK_LIBRARIES(akonaditray ${KNOTIFICATIONITEM_LIBRARIES} ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS} ${AKONADI_COMMON_LIBRARIES} ${KDE4_AKONADI_LIBS})

INSTALL(TARGETS akonaditray ${INSTALL_TARGETS_DEFAULT_ARGS})
INSTALL(FILES  akonaditray.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
