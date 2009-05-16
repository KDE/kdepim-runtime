project(
		akonadi_mailfilter_agent
	)

include_directories(
		${CMAKE_SOURCE_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}
		${Boost_INCLUDE_DIR}
	)

set(
		AKONADI_MAILFILTER_AGENT_SRCS
		agent.cpp
		main.cpp
	)

kde4_add_executable(
		akonadi_mailfilter_agent
		RUN_UNINSTALLED
		${AKONADI_MAILFILTER_AGENT_SRCS}
	)

target_link_libraries(
		akonadi_mailfilter_agent
		${KDE4_AKONADI_LIBS}
		ksieve
		${KDE4_KDECORE_LIBS}
		${QT_QTCORE_LIBRARY}
		${QT_QTDBUS_LIBRARY}
	)

set(
		CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}"
	)



#target_link_libraries(akonadi_mailfilter_agent ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KMIME_LIBS} )

install(
		TARGETS
		akonadi_mailfilter_agent
		${INSTALL_TARGETS_DEFAULT_ARGS}
	)
	
install(
		FILES
		mailfilteragent.desktop
		DESTINATION
		"${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
	)

