
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/shared
  ${CMAKE_CURRENT_BINARY_DIR}/shared
)

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourcebase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialogbase.cpp
)

set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)

if ( XSLTPROC_EXECUTABLE )
  add_subdirectory( knut )
  add_subdirectory( ical )
  add_subdirectory( maildir )
  add_subdirectory( nntp )
  add_subdirectory( vcard )
  add_subdirectory( vcarddir )
  add_subdirectory( localbookmarks )
  add_subdirectory( kabc )
  add_subdirectory( kcal )
  add_subdirectory( imap )
  add_subdirectory( nepomuktag )
  add_subdirectory( distlist )
  add_subdirectory(birthdays)
endif( XSLTPROC_EXECUTABLE )

#This is experimental and probably experiencing bit-rot.
#Also, there is a GSoC project for it.
#So, comment out for now -- talk to Brad Hards <bradh@frogmouth.net> for more info.
#if(UNIX)
#   macro_optional_find_package(OpenChange)
#   macro_log_feature(LIBMAPI_FOUND "libmapi" "OpenChange MAPI Client Library" "http://www.openchange.org" FALSE "" "Needed to support OpenChange / Exchange servers")
#
#if(LIBMAPI_FOUND)
#  add_subdirectory(openchange)
#endif(LIBMAPI_FOUND)
#
#endif(UNIX)
