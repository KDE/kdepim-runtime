
if(KDE4_BUILD_TESTS)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}
)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadiplugin_test _source _libs)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_automoc(${_test})
  kde4_add_executable(${_name} RUN_UNINSTALLED ${_test})
  target_link_libraries(${_name} akonadi ${QT_QTTEST_LIBRARY} ${_libs})
  add_test( akonadiplugin-${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
endmacro(add_akonadiplugin_test)

# qtestlib unit tests
add_akonadiplugin_test(mailserializertest.cpp ${KDE4_KMIME_LIBS})

endif(KDE4_BUILD_TESTS)

