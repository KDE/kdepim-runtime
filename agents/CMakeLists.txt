add_subdirectory( mailthreader )
add_subdirectory( strigifeeder )

if( Nepomuk_FOUND AND NOT Q_WS_MAC )

  #The following code to compute SOPRANO_PLUGIN_RAPTORPARSER_FOUND is lifted
  #from KDE4.2's FindSoprano.cmake. We have this code here for backwards
  #compatibility, and can be removed sometime after KDE 4.3 is released.
  find_path(_PLUGIN_DIR 
    NAMES
    soprano/plugins
    PATHS
    ${SHARE_INSTALL_PREFIX} /usr/share /usr/local/share
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    )
  set(_PLUGIN_DIR "${_PLUGIN_DIR}/soprano/plugins")

  if(EXISTS ${_PLUGIN_DIR}/raptorparser.desktop)
    set(SOPRANO_PLUGIN_RAPTORPARSER_FOUND TRUE)
  endif(EXISTS ${_PLUGIN_DIR}/raptorparser.desktop)

  if(SOPRANO_PLUGIN_RAPTORPARSER_FOUND)
    add_subdirectory( nie )
    add_subdirectory( nepomuk_email_feeder )
    add_subdirectory( nepomuk_contact_feeder )
  endif(SOPRANO_PLUGIN_RAPTORPARSER_FOUND)

endif( Nepomuk_FOUND AND NOT Q_WS_MAC )
