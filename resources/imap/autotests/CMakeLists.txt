kde_enable_exceptions()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

# if kdepimlibs was built without -DKDE4_BUILD_TESTS, kimaptest doesn't exist.
find_path(KIMAPTEST_INCLUDE_DIR NAMES kimaptest/fakeserver.h)
find_library(KIMAPTEST_LIBRARY NAMES kimaptest)

if(KIMAPTEST_INCLUDE_DIR AND KIMAPTEST_LIBRARY)
  MACRO(IMAP_RESOURCE_UNIT_TESTS)
    FOREACH(_testname ${ARGN})
      include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
      kde4_add_unit_test(${_testname} TESTNAME imap-${_testname} NOGUI ${_testname}.cpp
			dummypasswordrequester.cpp
			dummyresourcestate.cpp
			imaptestbase.cpp
			)
      
      target_link_libraries(${_testname} 
            KF5::Imap Qt5::Gui ${KIMAPTEST_LIBRARY} Qt5::Test imapresource)
      add_definitions(-DTEST_DATA="\\"${CMAKE_CURRENT_SOURCE_DIR}\\"")
    ENDFOREACH(_testname)
  ENDMACRO(IMAP_RESOURCE_UNIT_TESTS)

  IMAP_RESOURCE_UNIT_TESTS(
    testresourcetask
    testsessionpool

    testaddcollectiontask
    testadditemtask
    testchangecollectiontask
    testchangeitemtask
    testexpungecollectiontask
    testmovecollectiontask
    testmoveitemstask
    testremovecollectionrecursivetask
    testremoveitemstask
    testretrievecollectionmetadatatask
    testretrievecollectionstask
    testretrieveitemtask
    testretrieveitemstask
  )

endif()

