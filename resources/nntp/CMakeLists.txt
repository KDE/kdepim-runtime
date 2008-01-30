include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( nntpresource_SRCS
  nntpcollectionattribute.cpp
  nntpresource.cpp
  configdialog.cpp
  settings.cpp
  main.cpp
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

install( FILES nntpresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(nntpresource_SRCS configdialog.ui)
kde4_add_kcfg_files(nntpresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_nntp_resource RUN_UNINSTALLED ${nntpresource_SRCS})

target_link_libraries(akonadi_nntp_resource akonadi ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBRARY} ${KDE4_KMIME_LIBS})

install(TARGETS akonadi_nntp_resource DESTINATION ${BIN_INSTALL_DIR})
