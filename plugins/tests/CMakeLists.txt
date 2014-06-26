set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


include_directories(
    ${kdepim-runtime_SOURCE_DIR}/plugins
    ${kdepim-runtime_BINARY_DIR}/plugins
    ${Boost_INCLUDE_DIR}
)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadiplugin_test _source _libs _additionalSources)
  set(_test ${_source})
  get_filename_component(_name ${_source} NAME_WE)
  set(srcs ${_test} ${_additionalSources})
  kde4_add_unit_test(${_name} TESTNAME akonadiplugin-${_name} ${srcs})
  target_link_libraries(${_name} KF5::AkonadiCore KF5::AkonadiMime 
                         ${QT_QTTEST_LIBRARY} ${AKONADI_COMMON_LIBRARIES}
                        ${AKONADI_COMMON_LIBRARIES} ${_libs})
endmacro()

# qtestlib unit tests
add_akonadiplugin_test(mailserializertest.cpp "KF5::Mime" "")
add_akonadiplugin_test(mailserializerplugintest.cpp "KF5::Mime" "")

add_akonadiplugin_test(kcalcoreserializertest.cpp "KF5::CalendarCore" "")

add_akonadiplugin_test(addresseeserializertest.cpp "KF5::Abc;${KDEPIMLIBS_AKONADI_KABC_LIBS}" "../akonadi_serializer_addressee.cpp")
