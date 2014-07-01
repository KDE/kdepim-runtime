macro(_add_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadi-${_name} ${_test})
  target_link_libraries(${_name} Qt5::Test   
                        KF5::AkonadiCore KF5::Imap)
endmacro()

_add_test( collectionannotationattributetest.cpp )
_add_test( imapaclattributetest.cpp )
