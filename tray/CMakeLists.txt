project(tray)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory(icons)

find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)


set(tray_sources main.cpp dock.cpp backup.cpp backupassistant.cpp restore.cpp restoreassistant.cpp global.cpp)

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/dock.h
                             org.freedesktop.akonaditray.xml )
qt5_add_dbus_adaptor( tray_sources org.freedesktop.akonaditray.xml dock.h Dock )

kde4_add_app_icon(tray_sources "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-app-akonaditray.png")

add_executable(akonaditray ${tray_sources})

target_link_libraries(akonaditray KF5::KIOCore KF5::AkonadiCore)

install(TARGETS akonaditray ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS  akonaditray.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
