# Libkolabxml
find_package(Libkolabxml 1.1 QUIET CONFIG)
set_package_properties(Libkolabxml PROPERTIES DESCRIPTION "Kolabxml" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab XML Format Schema Definitions Library is required to build the Kolab Groupware Resource")

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
    add_test( NAME akonadi-mysql-db-${_testname} COMMAND akonaditest -c ${kdepim-runtime_SOURCE_DIR}/resourcetester/tests/unittestenv/config-mysql-db.xml ${_resourcetester} -c ${CMAKE_CURRENT_BINARY_DIR}/${_script} )
  endif ()
endmacro( akonadi_add_resourcetest )



add_subdirectory( akonotes )
if (TARGET KF5::AlarmCalendar)
    add_subdirectory( kalarm )
endif()
add_subdirectory( contacts )
add_subdirectory( dav )
add_subdirectory( ical )
add_subdirectory( imap )
if (Libkolabxml_FOUND)
    find_package(Boost REQUIRED COMPONENTS thread system)
    add_subdirectory( kolab )
endif()

#Disable it as it seems Facebook disabled the login and event API has been crippled
#add_subdirectory( facebook )
add_subdirectory( maildir )

add_subdirectory( openxchange )
add_subdirectory( pop3 )

add_subdirectory( google )
add_subdirectory( google-groupware )

add_subdirectory( shared )
add_subdirectory( birthdays )
add_subdirectory( mixedmaildir )
add_subdirectory( mbox )
add_subdirectory( vcarddir )
add_subdirectory( icaldir )
add_subdirectory( vcard )
add_subdirectory( folderarchivesettings )
add_subdirectory( tomboynotes )
add_subdirectory( ews )
