project(nepomuk_interfaces)

install(FILES
  org.kde.nepomuk.DataManagement.xml
  org.kde.nepomuk.ResourceWatcher.xml
  org.kde.nepomuk.ResourceWatcherConnection.xml
  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
  )
