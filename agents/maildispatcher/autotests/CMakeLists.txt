include(ECMMarkAsTest)

find_package(Qt5Test CONFIG REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# Stolen from kdepimlibs/akonadi/tests
macro(add_akonadi_isolated_test _source)
  get_filename_component(_targetName ${_source} NAME_WE)
  set(_srcList ${_source} )

  add_executable(${_targetName} ${_srcList})
  ecm_mark_as_test(maildispatcher-${_targetName})
  target_link_libraries(${_targetName}
    Qt5::Test
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::MailTransportAkonadi
    KF5::Mime
    KF5::I18n
    KF5::ConfigGui
    Qt5::DBus
    Qt5::Widgets
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_targetName} LOCATION )
    set(_executable ${_loc}.bat)
  else ()
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_targetName})
  endif ()
  if (UNIX)
    set(_executable ${_executable}.shell)
  endif ()

  find_program(_testrunner akonaditest)

  if (KDEPIM_RUN_ISOLATED_TESTS)
    add_test( maildispatcheragent-${_targetName} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml ${_executable} )
  endif ()
endmacro(add_akonadi_isolated_test)



add_akonadi_isolated_test( aborttest.cpp )
add_akonadi_isolated_test( dupetest.cpp )

