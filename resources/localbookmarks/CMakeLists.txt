include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( localbookmarksresource_SRCS
  localbookmarksresource.cpp
)

install( FILES localbookmarksresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


kde4_add_kcfg_files(localbookmarksresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/localbookmarksresource.kcfg org.kde.Akonadi.LocalBookmarks.Settings)
qt4_add_dbus_adaptor(localbookmarksresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.LocalBookmarks.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_localbookmarks_resource RUN_UNINSTALLED ${localbookmarksresource_SRCS})

target_link_libraries(akonadi_localbookmarks_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${KDEPIMLIBS_KCAL_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS akonadi_localbookmarks_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

