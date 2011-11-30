project(libdatamanagement)

include_directories( interfaces )

set(datamanagement_SRC
  simpleresource.cpp
  simpleresourcegraph.cpp
  genericdatamanagementjob.cpp
  createresourcejob.cpp
  describeresourcesjob.cpp
  storeresourcesjob.cpp
  dbustypes.cpp
  datamanagement.cpp
  resourcewatcher.cpp
  abstracttimeoutdbusinterface.cpp
  datamanagementinterface.cpp
  kdbusconnectionpool.cpp
)

#the interfaces are directly copied from the buildfiles to avoid installing the interface multiple times
set(datamanagement_SRC ${datamanagement_SRC} interfaces/resourcewatcherconnectioninterface.cpp interfaces/resourcewatchermanagerinterface.cpp)

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
