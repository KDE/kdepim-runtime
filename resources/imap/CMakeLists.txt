include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${CMAKE_SOURCE_DIR}/akonadi_agent
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( imaplibresource_SRCS
  imaplibresource.cpp
  main.cpp
)

install( FILES imaplibresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_executable(akonadi_imaplib_resource RUN_UNINSTALLED ${imaplibresource_SRCS})

target_link_libraries(akonadi_imaplib_resource akonadi ${QT_QTDBUS_LIBRARIES})

install(TARGETS akonadi_imaplib_resource DESTINATION ${BIN_INSTALL_DIR})

