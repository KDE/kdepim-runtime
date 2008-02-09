include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${CMAKE_SOURCE_DIR}/maildir
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( maildirresource_SRCS
  maildirresource.cpp
  main.cpp
)

install( FILES maildirresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(maildirresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)
qt4_add_dbus_adaptor(maildirresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_maildir_resource RUN_UNINSTALLED ${maildirresource_SRCS})

target_link_libraries(akonadi_maildir_resource akonadi maildir ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBRARY} ${KDE4_KMIME_LIBS})

install(TARGETS akonadi_maildir_resource DESTINATION ${BIN_INSTALL_DIR})

