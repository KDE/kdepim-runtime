
#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

include_directories(
  ${kdepim-runtime_SOURCE_DIR}/libkdepim-copy
  ${CMAKE_CURRENT_SOURCE_DIR}/shared
  ${CMAKE_CURRENT_BINARY_DIR}/shared
)

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourcebase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialogbase.cpp
)

if (KDEPIM_MOBILE_UI)
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog_mobile.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
else ()
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog_desktop.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
endif ()

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionannotationsattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionflagsattribute.cpp
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapaclattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapquotaattribute.cpp
)

add_subdirectory( akonotes )
add_subdirectory( kalarm )
add_subdirectory( contacts )
macro_optional_add_subdirectory( dav )
add_subdirectory( ical )
add_subdirectory( imap )
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory( kabc )
  add_subdirectory( kcal )
endif()
macro_optional_add_subdirectory( kdeaccounts )
if (Libkolab_FOUND AND Libkolabxml_FOUND)
    macro_optional_add_subdirectory( kolabproxy )
    add_subdirectory( kolab )
endif()

macro_optional_add_subdirectory( localbookmarks )

add_subdirectory( maildir )

macro_optional_add_subdirectory( openxchange )
add_subdirectory( pop3 )

if( LibKGAPI2_FOUND )
  add_subdirectory( google )
  # Disabled in KDE 4.14 - too many issues for stable release
  #add_subdirectory( gmail )
endif()

if( LibKFbAPI_FOUND )
  add_subdirectory( facebook )
endif()

add_subdirectory( shared )
add_subdirectory( birthdays )
add_subdirectory( mixedmaildir )
add_subdirectory( mailtransport_dummy )
add_subdirectory( mbox )
add_subdirectory( nntp )
add_subdirectory( vcarddir )
add_subdirectory( icaldir )
add_subdirectory( vcard )
add_subdirectory( folderarchivesettings )
