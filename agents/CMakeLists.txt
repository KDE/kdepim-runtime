add_subdirectory( mailthreader )

if( Nepomuk_FOUND AND NOT Q_WS_MAC )
   message(STATUS "does not compile for over a week, fix it or remove it!!!!!!!!!")
   #  add_subdirectory( nepomuk_contact_feeder )
endif( Nepomuk_FOUND AND NOT Q_WS_MAC )
