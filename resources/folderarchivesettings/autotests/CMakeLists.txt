
# Convenience macro to add unit tests.
macro(folderarchive_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable(${_name} ${_test} ${_name}.h)
  add_test(NAME ${_name} COMMAND ${_name})
  ecm_mark_as_test(folderararchive-${_name})

  target_link_libraries(${_name} Qt::Test Qt::Widgets  KPim6::AkonadiCore KF6::ConfigCore folderarchivesettings)
endmacro()

folderarchive_test(folderarchiveaccountinfotest.cpp)
folderarchive_test(folderarchivecomboboxtest.cpp)
