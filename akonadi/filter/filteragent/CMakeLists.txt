project(
		akonadi_filter_agent
	)

include_directories(
		${Boost_INCLUDE_DIR}
	)

# This path emulates the KDE include path when the library headers
# are actually installed.

include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}/../
	)

# we need these because we're including in-tree stuff which isn't installed yet

include_directories(
		${CMAKE_CURRENT_BINARY_DIR}/../
		${CMAKE_CURRENT_BINARY_DIR}/../akonadi/filter/
		${CMAKE_CURRENT_BINARY_DIR}/../akonadi/filter/ui/
	)


set(
		AKONADI_MAILFILTER_AGENT_SRCS
		agent.cpp
		engine.cpp
		main.cpp
	)


qt4_add_dbus_adaptor(
		AKONADI_MAILFILTER_AGENT_SRCS
		org.freedesktop.Akonadi.FilterAgent.xml
		agent.h
		FilterAgent
	)


kde4_add_executable(
		akonadi_filter_agent
		RUN_UNINSTALLED
		${AKONADI_MAILFILTER_AGENT_SRCS}
	)

target_link_libraries(
		akonadi_filter_agent
		akonadi-filter
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


install(
		FILES
			org.freedesktop.Akonadi.FilterAgent.xml
		DESTINATION
			${DBUS_INTERFACES_INSTALL_DIR}
	)


#target_link_libraries(akonadi_filter_agent ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KMIME_LIBS} )

install(
		TARGETS
			akonadi_filter_agent
		${INSTALL_TARGETS_DEFAULT_ARGS}
	)
	
install(
		FILES
			filteragent.desktop
		DESTINATION
			"${CMAKE_INSTALL_PREFIX}/share/akonadi/agents"
	)

