add_subdirectory( mailthreader )
add_subdirectory( strigifeeder )

if( Nepomuk_FOUND AND NOT Q_WS_MAC )

  #The following code to compute SOPRANO_PLUGIN_RAPTORPARSER_FOUND is lifted
  #from KDE4.2's FindSoprano.cmake. We have this code here for backwards
  #compatibility, and can be removed sometime after KDE 4.3 is released.
  set(SOPRANO_PLUGIN_DIR ${KDE4_DATA_DIR})
  string(REGEX REPLACE "apps.*$" "" SOPRANO_PLUGIN_DIR ${SOPRANO_PLUGIN_DIR})
  set(SOPRANO_PLUGIN_DIR "${SOPRANO_PLUGIN_DIR}/soprano/plugins")

  if(EXISTS ${SOPRANO_PLUGIN_DIR}/raptorparser.desktop)
    set(SOPRANO_PLUGIN_RAPTORPARSER_FOUND TRUE)
  endif(EXISTS ${SOPRANO_PLUGIN_DIR}/raptorparser.desktop)

  if(SOPRANO_PLUGIN_RAPTORPARSER_FOUND)
    add_subdirectory( nie )
    add_subdirectory( nepomuk_email_feeder )
    add_subdirectory( nepomuk_contact_feeder )
  endif(SOPRANO_PLUGIN_RAPTORPARSER_FOUND)

endif( Nepomuk_FOUND AND NOT Q_WS_MAC )
