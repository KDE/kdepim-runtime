include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( nntpresource_SRCS
  nntpcollectionattribute.cpp
  nntpresource.cpp
  main.cpp
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

install( FILES nntpresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc(${nntpresource_SRCS})

kde4_add_executable(akonadi_nntp_resource RUN_UNINSTALLED ${nntpresource_SRCS})

target_link_libraries(akonadi_nntp_resource akonadi ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBRARY} ${KDE4_KMIME_LIBS})

install(TARGETS akonadi_nntp_resource DESTINATION ${BIN_INSTALL_DIR})
