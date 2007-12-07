add_subdirectory( ical )
add_subdirectory( knut )
add_subdirectory( vcard )
add_subdirectory( nntp )
add_subdirectory( maildir )
add_subdirectory( localbookmarks )

#This is experimental and probably experiencing bit-rot.
#So, comment out for now -- talk to Brad Hards <bradh@frogmouth.net> for more info.
#if(UNIX)
#   macro_optional_find_package(OpenChange)
#   macro_log_feature(LIBMAPI_FOUND "libmapi" "OpenChange MAPI Client Library" "http://www.openchange.org" FALSE "" "Needed to support OpenChange / Exchange servers")

#   if(LIBMAPI_FOUND)
#      add_subdirectory(openchange)
#   endif(LIBMAPI_FOUND)

#endif(UNIX)
