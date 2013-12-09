add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

macro(kdepim_add_agent _target)
    if(KDEPIM_BUILD_AGENTS_AS_PLUGINS)
      add_definitions(-DKDEPIM_PLUGIN_AGENT)
      kde4_add_plugin(${_target} ${ARGN})
      install( TARGETS ${_target} DESTINATION ${PLUGIN_INSTALL_DIR}/ )
    else()
      kde4_add_executable(${_target} ${ARGN})
      install(TARGETS ${_target} ${INSTALL_TARGETS_DEFAULT_ARGS})
    endif()
endmacro()

add_subdirectory( maildispatcher )
add_subdirectory( newmailnotifier )
add_subdirectory( migration )
add_subdirectory( invitations )

if( NOT KDEPIM_NO_NEPOMUK )
    include_directories( ${CMAKE_CURRENT_BINARY_DIR}/ontologies ontologies ${SOPRANO_INCLUDE_DIR} )

    add_subdirectory( nepomukfeeder )
    add_subdirectory( ontologies )

endif()
