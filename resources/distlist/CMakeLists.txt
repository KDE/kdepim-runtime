
include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( distlistresource_SRCS
  ${AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES}
  distlistresource.cpp
)

install( FILES distlistresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(distlistresource_SRCS ${AKONADI_SINGLEFILERESOURCE_SHARED_UI})
kde4_add_kcfg_files(distlistresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/distlistresource.kcfg org.kde.Akonadi.DistList.Settings)
qt4_add_dbus_adaptor(distlistresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.DistList.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_distlist_resource RUN_UNINSTALLED ${distlistresource_SRCS})

target_link_libraries(akonadi_distlist_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS akonadi_distlist_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
