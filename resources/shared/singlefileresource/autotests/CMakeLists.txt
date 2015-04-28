include(ECMAddTests)

find_package(Qt5Test CONFIG REQUIRED)

macro(_add_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(knotes-${_name})
  target_link_libraries(${_name} Qt5::Test   
                        KF5::AkonadiCore KF5::IMAP)
endmacro()

_add_test( collectionannotationattributetest.cpp )
_add_test( imapaclattributetest.cpp )
