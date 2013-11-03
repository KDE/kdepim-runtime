
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

foreach( _file ${kolabproxyresource_shared_SRCS} )
  list( APPEND kolabproxy_shared_relative_SRCS "../${_file}" )
endforeach()

kde4_add_library(kolabresourcetest STATIC ${kolabproxy_shared_relative_SRCS} ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES})

kde4_add_unit_test(kolabconvertertest ${kolabproxy_shared_relative_SRCS} ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES} kolabconvertertest.cpp)

target_link_libraries(kolabconvertertest ${Libkolab_LIBRARIES} ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTXML_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTTEST_LIBRARY}  kdepim-copy ${KDEPIMLIBS_KCALCORE_LIBS})


#akonadi_add_resourcetest( imap-kolab imaptest-kolab.es )
#akonadi_add_resourcetest( imap-dovecot imaptest-dovecot.es )
#akonadi_add_resourcetest( kolab kolabtest.es )

set(KDEPIMLIBS_RUN_ISOLATED_TESTS ${KDEPIM_RUN_ISOLATED_TESTS})
set(KDEPIMLIBS_TESTS_MYSQL TRUE CACHE BOOL "Run tests with mysql")
set(KDEPIMLIBS_TESTS_SQLITE FALSE CACHE BOOL "Run tests with sqlite")

# convenience macro to add akonadi testrunner unit-tests
# use AkonadiMacros from kdepimlibs (and make sure the PATH is still exportet)
macro(add_akonadi_isolated_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test})
  target_link_libraries(${_name} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES} kolabresourcetest)

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
  else()
    set(_executable ${RUNTIME_OUTPUT_DIRECTORY}/${_name})
  endif()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_name})
    else()
      set(_executable ${_executable}.shell)
    endif()
  endif()

  find_program(_testrunner akonaditest)

  if ( KDEPIMLIBS_TESTS_XML )
    set( MYSQL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-db-${_name}.xml )
    set( MYSQL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/mysql-fs-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_DB -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-db-${_name}.xml )
    set( POSTGRESL_EXTRA_OPTIONS_FS -xml -o ${TEST_RESULT_OUTPUT_PATH}/postgresql-fs-${_name}.xml )
    set( SQLITE_EXTRA_OPTIONS -xml -o ${TEST_RESULT_OUTPUT_PATH}/sqlite-${_name}.xml )
  endif()

  if ( KDEPIMLIBS_RUN_ISOLATED_TESTS )
    if ( KDEPIMLIBS_TESTS_MYSQL )
      find_program( MYSQLD_EXECUTABLE mysqld /usr/sbin /usr/local/sbin /usr/libexec /usr/local/libexec /opt/mysql/libexec /usr/mysql/bin )
      if ( MYSQLD_EXECUTABLE )
        add_test( akonadi-mysql-db-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-mysql-db.xml ${_executable} ${MYSQL_EXTRA_OPTIONS_DB} )
        set_tests_properties( akonadi-mysql-db-${_name} PROPERTIES ENVIRONMENT "PATH=${RUNTIME_OUTPUT_DIRECTORY}/..:$ENV{PATH}" )
      endif()
    endif()

    if ( KDEPIMLIBS_TESTS_SQLITE )
      add_test( akonadi-sqlite-${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config-sqlite-db.xml ${_executable}
        ${SQLITE_EXTRA_OPTIONS} )
      set_tests_properties( akonadi-sqlite-${_name} PROPERTIES ENVIRONMENT "PATH=${RUNTIME_OUTPUT_DIRECTORY}/..:$ENV{PATH}" )
    endif()
  endif()
endmacro()

add_akonadi_isolated_test(proxyintegrationtest.cpp)

