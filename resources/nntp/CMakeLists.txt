include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/resources/include
	${QT_QTDBUS_INCLUDE_DIR}
)

set( nntpresource_SRCS
  nntpresource.cpp
  main.cpp
)

install( FILES nntpresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc(${nntpresource_SRCS})

kde4_add_executable(akonadi_nntp_resource RUN_UNINSTALLED ${nntpresource_SRCS})

target_link_libraries(akonadi_nntp_resource akonadi akonadiresources ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_nntp_resource DESTINATION ${BIN_INSTALL_DIR})
