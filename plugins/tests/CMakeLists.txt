set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
    ${kdepim-runtime_SOURCE_DIR}/plugins
    ${kdepim-runtime_BINARY_DIR}/plugins
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

add_akonadiplugin_test(kcalcoreserializertest.cpp "${KDEPIMLIBS_KCALCORE_LIBS}")

if (KDEPIMLIBS_KCAL_LIBS)
  add_akonadiplugin_test(kcalserializertest.cpp "${KDEPIMLIBS_KCAL_LIBS}")
endif ()


macro(add_simple_test _testName)
  set(_targetName ${_testName})
  set(_srcList ${ARGN})
  kde4_add_unit_test(${_targetName} ${_srcList})
  target_link_libraries(${_targetName} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_SYNDICATION_LIBS} ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KRSS_LIBS})
endmacro(add_simple_test)

set(testxmlitemserializer_SRCS
    testxmlitemserializer.cpp
)

if(KRSS_ENABLE_QDATASTREAM_SERIALIZER)
    set(testxmlitemserializer_SRCS
        ${testxmlitemserializer_SRCS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../rssitemserializer_qdatastream.cpp
    )
else(KRSS_ENABLE_QDATASTREAM_SERIALIZER)
if(KRSS_ENABLE_JSON_SERIALIZER)
   set(testxmlitemserializer_SRCS
       ${testxmlitemserializer_SRCS}
       ${CMAKE_CURRENT_SOURCE_DIR}/../rssitemserializer_json.cpp
   )
   set(KRSS_EXTRA_LIBS qjson)
else(KRSS_ENABLE_JSON_SERIALIZER)
   set(testxmlitemserializer_SRCS
       ${testxmlitemserializer_SRCS}
       ${CMAKE_CURRENT_SOURCE_DIR}/../rssitemserializer_xml.cpp
   )
endif(KRSS_ENABLE_JSON_SERIALIZER)
endif(KRSS_ENABLE_QDATASTREAM_SERIALIZER)

add_simple_test(testxmlitemserializer ${testxmlitemserializer_SRCS})
