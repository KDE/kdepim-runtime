include_directories( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                     ${QT_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
                     ${LIBKNOTIFICATIONITEM-1_INCLUDE_DIR} )

set(tray_sources main.cpp dock.cpp backup.cpp backupassistant.cpp restore.cpp restoreassistant.cpp global.cpp)

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/dock.h
                             org.freedesktop.akonaditray.xml )
qt4_add_dbus_adaptor( tray_sources org.freedesktop.akonaditray.xml dock.h Dock )

kde4_add_app_icon(tray_sources "${KDE4_ICON_DIR}/oxygen/*/apps/akonadi.png")

kde4_add_executable(akonaditray ${tray_sources})

target_link_libraries(akonaditray ${LIBKNOTIFICATIONITEM-1_LIBRARIES} ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS} ${AKONADI_COMMON_LIBRARIES} ${KDEPIMLIBS_AKONADI_LIBS})

install(TARGETS akonaditray ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES  akonaditray.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
