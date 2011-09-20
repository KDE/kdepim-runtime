macro(kdepim_add_agent _target)
    if(KDEPIM_BUILD_AGENTS_AS_PLUGINS)
      add_definitions(-DKDEPIM_PLUGIN_AGENT)
      kde4_add_plugin(${_target} ${ARGN})
      install( TARGETS ${_target} DESTINATION ${PLUGIN_INSTALL_DIR}/ )
    else(KDEPIM_BUILD_AGENTS_AS_PLUGINS)
      kde4_add_executable(${_target} ${ARGN})
      install(TARGETS ${_target} ${INSTALL_TARGETS_DEFAULT_ARGS})
    endif(KDEPIM_BUILD_AGENTS_AS_PLUGINS)
endmacro(kdepim_add_agent)

add_subdirectory( maildispatcher )
add_subdirectory( newmailnotifier )

if( STRIGI_FOUND AND AKONADI_USE_STRIGI_SEARCH )
add_subdirectory( strigifeeder )
endif( STRIGI_FOUND AND AKONADI_USE_STRIGI_SEARCH )

if(NOT WINCE)
add_subdirectory( invitations )
add_subdirectory( calendarsearch )

if( NOT KDEPIM_NO_NEPOMUK AND Nepomuk_FOUND AND STRIGI_FOUND )
    include_directories( ${CMAKE_CURRENT_BINARY_DIR}/ontologies ontologies ${SOPRANO_INCLUDE_DIR} ${STRIGI_INCLUDE_DIR} )

    add_subdirectory( nepomukfeeder )
    add_subdirectory( ontologies )

endif( NOT KDEPIM_NO_NEPOMUK AND Nepomuk_FOUND AND STRIGI_FOUND )
endif(NOT WINCE)
