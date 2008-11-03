set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

### convenience macro 
macro(add_imaplib_test _source)
  set(_test ${_source} fakeserver.cpp )
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME imaplib-${_name} ${_test})
  target_link_libraries(${_name} imap ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDEUI_LIBS} )
endmacro(add_imaplib_test)

### tests
add_imaplib_test(imaplibtest.cpp)

