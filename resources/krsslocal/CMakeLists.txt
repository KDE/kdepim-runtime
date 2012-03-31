project(krsslocal)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
include_directories(${krss_SOURCE_DIR})

########### next target ###############

set( krsslocalresource_SRCS
  krsslocalresource.cpp
  util.cpp
  opmlparser.cpp
  rssitemsync.cpp
)

kde4_add_kcfg_files(krsslocalresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/krsslocalresource.kcfg org.kde.Akonadi.RssLocal.Settings)
qt4_add_dbus_adaptor(krsslocalresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.RssLocal.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_krsslocal_resource RUN_UNINSTALLED ${krsslocalresource_SRCS})

target_link_libraries(akonadi_krsslocal_resource
                      ${KDE4_KDECORE_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDEPIMLIBS_SYNDICATION_LIBS}
                      ${KDEPIMLIBS_KRSS_LIBS}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
)

install(TARGETS akonadi_krsslocal_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES krsslocalresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
