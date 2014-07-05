project(kolabproxy)

include_directories(
  ${kdepim-runtime_SOURCE_DIR}
  ${QT_QTDBUS_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${Libkolab_INCLUDES}
  ${Libkolabxml_INCLUDES}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -fPIC")

########### next target ###############

set(kolabproxyresource_shared_SRCS
  kolabhandler.cpp
  addressbookhandler.cpp
  incidencehandler.cpp
  calendarhandler.cpp
  freebusyupdatehandler.cpp
  taskshandler.cpp
  journalhandler.cpp
  notehandler.cpp
  setupdefaultfoldersjob.cpp
  kolabdefs.cpp
  upgradejob.cpp
  imapitemaddedjob.cpp
  imapitemremovedjob.cpp
  itemaddedjob.cpp
  itemchangedjob.cpp
  handlermanager.cpp
  revertitemchangesjob.cpp
)

set(kolabproxyresource_SRCS
  kolabproxyresource.cpp
  collectiontreebuilder.cpp
  setupkolab.cpp
  ${kolabproxyresource_shared_SRCS}
  ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
)

qt5_add_dbus_adaptor(kolabproxyresource_SRCS org.freedesktop.Akonadi.kolabproxy.xml  kolabproxyresource.h KolabProxyResource)

install(FILES kolabproxyresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

kconfig_add_kcfg_files(kolabproxyresource_SRCS settings.kcfgc)

ki18n_wrap_ui(kolabproxyresource_SRCS kolabsettings.ui changeformat.ui)

kcfg_generate_dbus_interface(
  ${CMAKE_CURRENT_SOURCE_DIR}/kolabproxyresource.kcfg
  org.kde.Akonadi.kolabproxy.Settings
)

qt5_add_dbus_adaptor(kolabproxyresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.kolabproxy.Settings.xml
  settings.h Settings
)

if(RUNTIME_PLUGINS_STATIC)
  add_definitions(-DRUNTIME_PLUGINS_STATIC)
endif()

add_executable(akonadi_kolabproxy_resource ${kolabproxyresource_SRCS})
install(FILES akonadi_kolabproxy_resource.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR} )

if(Q_WS_MAC)
  set_target_properties(akonadi_kolabproxy_resource PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template
  )
  set_target_properties(akonadi_kolabproxy_resource PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.kolabproxy"
  )
  set_target_properties(akonadi_kolabproxy_resource PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Kolabproxy Resource"
  )
endif()

target_link_libraries(akonadi_kolabproxy_resource
  kdepim-copy
  KF5::AkonadiMime
  KF5::AkonadiCore
  KF5::Abc
  KF5::CalendarCore
  KF5::Mime
  KF5::KIOCore
  
  ${QT_QTXML_LIBRARY}
  ${Libkolab_LIBRARIES}
  ${Libkolabxml_LIBRARIES}
)

if(RUNTIME_PLUGINS_STATIC)
  target_link_libraries(akonadi_kolabproxy_resource
    akonadi_serializer_addressee
    akonadi_serializer_contactgroup
    akonadi_serializer_kcalcore
    akonadi_serializer_mail
  )
endif()

install(TARGETS akonadi_kolabproxy_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

ecm_install_icons(ICONS hi64-apps-kolab.png DESTINATION ${ICON_INSTALL_DIR})

add_subdirectory(tests)
add_subdirectory(wizard)
