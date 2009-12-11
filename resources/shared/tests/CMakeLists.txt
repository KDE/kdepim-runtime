macro(_add_test _source)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadi-${_name} ${_test})
  target_link_libraries(${_name} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDECORE_LIBS} ${AKONADI_COMMON_LIBRARIES} ${KDEPIMLIBS_AKONADI_LIBS})
endmacro(_add_test)

_add_test( collectionannotationattributetest.cpp )
