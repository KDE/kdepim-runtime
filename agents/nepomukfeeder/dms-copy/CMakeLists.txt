project(libdatamanagement)

add_subdirectory( interfaces )

set(datamanagement_SRC
  simpleresource.cpp
  simpleresourcegraph.cpp
  genericdatamanagementjob.cpp
  createresourcejob.cpp
  describeresourcesjob.cpp
  dbustypes.cpp
  datamanagement.cpp
  resourcewatcher.cpp
)

set(datamanagementservice_dbus_desc interfaces/org.kde.nepomuk.DataManagement.xml)

set_source_files_properties(
  ${datamanagementservice_dbus_desc}
  PROPERTIES
  INCLUDE "dbustypes.h")

qt4_add_dbus_interface(datamanagement_SRC
  interfaces/org.kde.nepomuk.ResourceWatcherConnection.xml
  resourcewatcherconnectioninterface)

qt4_add_dbus_interface(datamanagement_SRC
  interfaces/org.kde.nepomuk.ResourceWatcher.xml
  resourcewatchermanagerinterface)

qt4_add_dbus_interface(datamanagement_SRC
  ${datamanagementservice_dbus_desc}
  datamanagementinterface)

kde4_add_library(nepomukdatamanagement-copy SHARED
  ${datamanagement_SRC})

target_link_libraries(nepomukdatamanagement-copy
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
  ${KDE4_KDECORE_LIBS})

 install(TARGETS nepomukdatamanagement-copy
   DESTINATION ${LIB_INSTALL_DIR})
