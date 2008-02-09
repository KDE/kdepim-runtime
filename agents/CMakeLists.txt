add_subdirectory( mailthreader )
add_subdirectory( strigifeeder )

if( Nepomuk_FOUND AND NOT Q_WS_MAC )
  add_subdirectory( nepomukfeeder )
   message(STATUS "does not compile for over a week, fix it or remove it!!!!!!!!!")
   #  add_subdirectory( nepomuk_contact_feeder )
endif( Nepomuk_FOUND AND NOT Q_WS_MAC )
