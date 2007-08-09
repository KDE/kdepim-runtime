include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( localbookmarksresource_SRCS
  localbookmarksresource.cpp
  main.cpp
)

install( FILES localbookmarksresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )


kde4_add_executable(akonadi_localbookmarks_resource RUN_UNINSTALLED ${localbookmarksresource_SRCS})

target_link_libraries(akonadi_localbookmarks_resource akonadi ${QT_QTDBUS_LIBRARIES}  ${KDE4_KCAL_LIBS} ${KDE4_KIO_LIBRARY})

install(TARGETS akonadi_localbookmarks_resource DESTINATION ${BIN_INSTALL_DIR})

