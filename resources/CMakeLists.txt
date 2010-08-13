
include_directories(
  ${kdepim-runtime_SOURCE_DIR}/libkdepim-copy
  ${CMAKE_CURRENT_SOURCE_DIR}/shared
  ${CMAKE_CURRENT_BINARY_DIR}/shared
)

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourcebase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigwidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialogbase.cpp
)

if (KDEPIM_MOBILE_UI)
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigwidget_mobile.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
else (KDEPIM_MOBILE_UI)
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigwidget_desktop.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
endif (KDEPIM_MOBILE_UI)

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionannotationsattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionflagsattribute.cpp
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapaclattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapquotaattribute.cpp
)

add_subdirectory( akonotes )
add_subdirectory( birthdays )
add_subdirectory( contacts )
macro_optional_add_subdirectory( dav )
add_subdirectory( ical )
add_subdirectory( imap )
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory( kabc )
  add_subdirectory( kcal )
endif (KDEPIMLIBS_KRESOURCES_LIBS)
macro_optional_add_subdirectory( kdeaccounts )
add_subdirectory( knut )
add_subdirectory( kolabproxy )
macro_optional_add_subdirectory( localbookmarks )
add_subdirectory( mixedmaildir )
add_subdirectory( maildir )
add_subdirectory( mailtransport_dummy )
add_subdirectory( mbox )
macro_optional_add_subdirectory( microblog )
macro_optional_add_subdirectory( openxchange )
add_subdirectory( pop3 )

if ( NEPOMUK_FOUND )
  add_subdirectory( nepomuktag )
endif ( NEPOMUK_FOUND )

add_subdirectory( nntp )
add_subdirectory( vcard )
add_subdirectory( vcarddir )
add_subdirectory( shared )
