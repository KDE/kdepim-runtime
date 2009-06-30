project(
		akonadi_filter_agent
	)


# D-BUS related macros: this should be present in newer versions of CMake

include(UsePkgConfig)
 
MACRO(PKGCONFIG_GETVAR _package _var _output_variable)
  SET(${_output_variable})
 
  # if pkg-config has been found
  IF(PKGCONFIG_EXECUTABLE)
 
    EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --exists RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
 
    # and if the package of interest also exists for pkg-config, then get the information
    IF(NOT _return_VALUE)
 
      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable ${_var} OUTPUT_VARIABLE ${_output_variable} )
 
    ENDIF(NOT _return_VALUE)
 
  ENDIF(PKGCONFIG_EXECUTABLE)
 
ENDMACRO(PKGCONFIG_GETVAR _package _var _output_variable)
 
macro(dbus_add_activation_service _sources)
    PKGCONFIG_GETVAR(dbus-1 session_bus_services_dir _install_dir)
    foreach (_i ${_sources})
        get_filename_component(_service_file ${_i} ABSOLUTE)
        string(REGEX REPLACE "\\.service.*$" ".service" _output_file ${_i})
        set(_target ${CMAKE_CURRENT_BINARY_DIR}/${_output_file})
        configure_file(${_service_file} ${_target})
        install(FILES ${_target} DESTINATION ${_install_dir})
    endforeach (_i ${ARGN})
endmacro(dbus_add_activation_service _sources)

# EOF D-BUS Related macros

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
		AKONADI_FILTER_AGENT_SRCS
		datarfc822.cpp
		filteragent.cpp
		filterengine.cpp
		filterenginerfc822.cpp
		main.cpp
	)


qt4_add_dbus_adaptor(
		AKONADI_FILTER_AGENT_SRCS
		org.freedesktop.Akonadi.FilterAgent.xml
		filteragent.h
		FilterAgent
	)

# include(DBusMacros) <-- this should be added when DBusMacros are widely available in CMake
dbus_add_activation_service( org.freedesktop.Akonadi.Agent.akonadi_filter_agent.service.cmake )

kde4_add_executable(
		akonadi_filter_agent
		RUN_UNINSTALLED
		${AKONADI_FILTER_AGENT_SRCS}
	)

target_link_libraries(
		akonadi_filter_agent
		akonadi-filter
		akonadi-kmime
		${KDEPIMLIBS_AKONADI_LIBS}
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


#target_link_libraries(akonadi_filter_agent ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY}  ${STRIGIQTDBUSCLIENT_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS} )

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

