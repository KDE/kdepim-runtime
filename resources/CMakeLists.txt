
include_directories(
  ${kdepim-runtime_SOURCE_DIR}/libkdepim-copy
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

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionflagsattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionannotationsattribute.cpp
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapaclattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapquotaattribute.cpp
)

add_subdirectory( birthdays )
add_subdirectory( distlist )
add_subdirectory( ical )
add_subdirectory( imap )
add_subdirectory( kabc )
add_subdirectory( kcal )
add_subdirectory( knut )
add_subdirectory( kolabproxy )
add_subdirectory( localbookmarks )
add_subdirectory( maildir )
add_subdirectory( mbox )
add_subdirectory( microblog )

if ( NEPOMUK_FOUND )
  add_subdirectory( nepomuktag )
endif ( NEPOMUK_FOUND )

add_subdirectory( nntp )
add_subdirectory( vcard )
add_subdirectory( vcarddir )
add_subdirectory( shared )

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
