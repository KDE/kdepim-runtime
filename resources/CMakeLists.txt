project(resources)
cmake_minimum_required(VERSION 2.8.12)

set(AKONADICALENDAR_LIB_VERSION "4.78.0")
set(KMBOX_LIB_VERSION "4.78.0")


find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)

# Extra package
find_package(KF5GAPI "4.72.0" CONFIG)



# Xsltproc
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")

find_package(AccountsQt5 1.11 QUIET CONFIG)
set_package_properties(AccountsQt PROPERTIES DESCRIPTION "Qt bindings for accounts-sso" URL "https://code.google.com/p/accounts-sso/" TYPE OPTIONAL PURPOSE "Required to support AccountsQt in different resources")

find_package(SignOnQt5 8.56 QUIET CONFIG)
set_package_properties(SignOnQt PROPERTIES DESCRIPTION "SignOn Qt bindings for accounts-sso" URL "https://code.google.com/p/accounts-sso/" TYPE OPTIONAL PURPOSE "Required to support SignOn in different resources")

# Libkolab
find_package(KF5Kolab "1.0.0" QUIET CONFIG)
set_package_properties(Libkolab PROPERTIES DESCRIPTION "libkolab" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab Format libraries are required to build the Kolab Groupware Resource")

# Libkolabxml
find_package(Libkolabxml 1.0 QUIET CONFIG)
set_package_properties(Libkolabxml PROPERTIES DESCRIPTION "Kolabxml" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab XML Format Schema Definitions Library is required to build the Kolab Groupware Resource")

# Libkfbapi
find_package(KF5FbAPI 1.0 QUIET CONFIG)
set_package_properties(LibKFbAPI PROPERTIES DESCRIPTION "A library to access Facebook services" URL "http://projects.kde.org/libkfbapi" TYPE OPTIONAL PURPOSE "LibKFbAPI is required to build Akonadi resources to access Facebook's contacts, events, notes and posts" )

# KAccouunts bits
find_package(KAccounts CONFIG)
find_package(AccountsQt5 1.13 CONFIG)
find_package(SignOnQt5 8.57 CONFIG)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresource/
  ${CMAKE_CURRENT_BINARY_DIR}/shared/singlefileresource/
  ${CMAKE_CURRENT_SOURCE_DIR}/folderarchivesettings/
)


# resource tests
macro( akonadi_add_resourcetest _testname _script )
  if ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${EXECUTABLE_OUTPUT_PATH} )
  else ()
    set( _exepath ${kdepim-runtime_BINARY_DIR}/resourcetester )
  endif ()
  if (WIN32)
    set(_resourcetester ${_exepath}/resourcetester.bat)
  else ()
    set(_resourcetester ${_exepath}/resourcetester)
  endif ()
  if (UNIX)
    set(_resourcetester ${_resourcetester}.shell)
  endif ()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_script} ${CMAKE_CURRENT_BINARY_DIR}/${_script} COPYONLY)
  if (KDEPIM_RUN_ISOLATED_TESTS)
    add_test( akonadi-mysql-db-${_testname} akonaditest -c ${kdepim-runtime_SOURCE_DIR}/resourcetester/tests/unittestenv/config-mysql-db.xml ${_resourcetester} -c ${CMAKE_CURRENT_BINARY_DIR}/${_script} )
  endif ()
endmacro( akonadi_add_resourcetest )



#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory( akonotes )
add_subdirectory( kalarm )
add_subdirectory( contacts )
add_subdirectory( dav )
add_subdirectory( ical )
add_subdirectory( imap )
if (KF5Kolab_FOUND AND Libkolabxml_FOUND)
    add_subdirectory( kolabproxy )
    add_subdirectory( kolab )
endif()

add_subdirectory( maildir )

add_subdirectory( openxchange )
add_subdirectory( pop3 )

if( KF5GAPI_FOUND )
  add_subdirectory( google )
  # Disabled in KDE 4.14 - too many issues for stable release
  #add_subdirectory( gmail )
endif()

if (KF5FbAPI_FOUND AND KAccounts_FOUND AND AccountsQt5_FOUND)
  add_subdirectory(facebook)
endif()

add_subdirectory( shared )
add_subdirectory( birthdays )
add_subdirectory( mixedmaildir )
add_subdirectory( mbox )
add_subdirectory( vcarddir )
add_subdirectory( icaldir )
add_subdirectory( vcard )
add_subdirectory( folderarchivesettings )

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${resources_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

