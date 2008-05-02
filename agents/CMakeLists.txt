add_subdirectory( mailthreader )
add_subdirectory( strigifeeder )

if( Nepomuk_FOUND AND NOT Q_WS_MAC )
  add_subdirectory( nie )
  add_subdirectory( nepomuk_email_feeder )
  add_subdirectory( nepomuk_contact_feeder )
endif( Nepomuk_FOUND AND NOT Q_WS_MAC )
