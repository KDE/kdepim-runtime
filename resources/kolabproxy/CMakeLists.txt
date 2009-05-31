project(kolabproxy)

include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kolabproxyresource_SRCS
  kolabproxyresource.cpp
  contact.cpp
  event.cpp
  task.cpp
  journal.cpp
  incidence.cpp
  kolabbase.cpp
  kolabhandler.cpp
  addressbookhandler.cpp
  incidencehandler.cpp
  calendarhandler.cpp
  taskshandler.cpp
  journalhandler.cpp
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
)

install( FILES kolabproxyresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(kolabproxyresource_SRCS settings.kcfgc)

#kde4_add_ui_files(imapresource_SRCS setupserverview.ui)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kolabproxyresource.kcfg org.kde.Akonadi.kolabproxy.Settings)
qt4_add_dbus_adaptor(kolabproxyresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.kolabproxy.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_kolabproxy_resource RUN_UNINSTALLED ${kolabproxyresource_SRCS})

target_link_libraries(akonadi_kolabproxy_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_KCAL_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KMIME_LIBS} kdepim)

install(TARGETS akonadi_kolabproxy_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
