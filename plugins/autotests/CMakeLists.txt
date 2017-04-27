include(ECMAddTests)

find_package(Qt5Test CONFIG REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


include_directories(
    ${kdepim-runtime_SOURCE_DIR}/plugins
    ${kdepim-runtime_BINARY_DIR}/plugins
)

# convenience macro to add akonadi qtestlib unit-tests
macro(add_akonadiplugin_test _source _libs _additionalSources _target_included_directories)
  set(_test ${_source})
  set(srcs ${_test} ${_additionalSources})

  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${srcs} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(akonadiplugin-${_name})
  if(NOT ${_target_included_directories} STREQUAL "")
    target_include_directories(${_name} PRIVATE "$<BUILD_INTERFACE:$<TARGET_PROPERTY:${_target_included_directories},INCLUDE_DIRECTORIES>>")
  endif()
  target_link_libraries(${_name} KF5::AkonadiCore KF5::AkonadiMime
                         Qt5::Test KF5::AkonadiPrivate KF5::I18n
                        KF5::AkonadiPrivate ${_libs})
endmacro()

# qtestlib unit tests
add_akonadiplugin_test(mailserializertest.cpp "KF5::Mime" "../akonadi_serializer_mail_debug.cpp" "akonadi_serializer_mail")
add_akonadiplugin_test(mailserializerplugintest.cpp "KF5::Mime" "" "" )
add_akonadiplugin_test(kcalcoreserializertest.cpp "KF5::CalendarCore" "" "" )
add_akonadiplugin_test(addresseeserializertest.cpp "KF5::Contacts;KF5::AkonadiContact" "../akonadi_serializer_addressee.cpp" "")
