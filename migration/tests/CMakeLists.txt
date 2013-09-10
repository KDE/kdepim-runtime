project(migrationtests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# convenience macro to add akonadi testrunner unit-tests
macro(add_akonadi_isolated_test _source _additionalsources)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_executable(${_name} TEST ${_test} ${_additionalsources})
  target_link_libraries(${_name}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
    ${KDEPIMLIBS_KPIMTEXTEDIT_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_KCAL_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTXML_LIBRARY}
    maildir
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set(_executable ${_loc}.bat)
    set(_executable ${_loc}.bat)
  else ()
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_name})
  endif ()
  if (UNIX)
    if (APPLE)
      set(_executable ${_executable}.app/Contents/MacOS/${_name})
    else()
      set(_executable ${_executable}.shell)
    endif()
  endif ()
  
  find_program(_testrunner akonaditest)

  if ( KDEPIM_RUN_ISOLATED_TESTS )
    add_test( ${_name} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml ${_executable} )
  endif()
endmacro()

set(_srcList
  testnotesmigration.cpp
  ../kjots/kjotsmigrator.cpp
  ../kres/knotesmigrator.cpp
  ../kres/kresmigratorbase.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../kjots
  ${CMAKE_CURRENT_BINARY_DIR}/../kres
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  qt4_add_dbus_interface(_srcList
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )

add_akonadi_isolated_test( testnotesmigration.cpp "${_srcList}")

add_akonadi_isolated_test( testgidmigration.cpp "${MIGRATION_AKONADI_SHARED_SOURCES};../gid/gidmigrationjob.cpp" )

kde4_add_executable(testmigratorbase testmigratorbase.cpp ../migratorbase.cpp)
target_link_libraries(testmigratorbase
    gidmigration
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTTEST_LIBRARY}
)
add_test(testmigratorbase testmigratorbase)
