set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${CMAKE_CURRENT_BINARY_DIR}/../
    ${Boost_INCLUDE_DIR}
)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadiplugin_test _source _libs)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME akonadiplugin-${_name} ${_test})
  target_link_libraries(${_name} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDE4_KDECORE_LIBS}
                        ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${AKONADI_COMMON_LIBRARIES}
                        ${AKONADI_COMMON_LIBRARIES} ${_libs})
endmacro(add_akonadiplugin_test)

# qtestlib unit tests
add_akonadiplugin_test(mailserializertest.cpp "${KDEPIMLIBS_KMIME_LIBS}")
add_akonadiplugin_test(mailserializerplugintest.cpp "${KDEPIMLIBS_KMIME_LIBS}")
