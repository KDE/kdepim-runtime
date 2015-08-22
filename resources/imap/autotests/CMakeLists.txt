kde_enable_exceptions()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(ECMMarkAsTest)

find_package(Qt5Test CONFIG REQUIRED)


# if kdepimlibs was built without -DKDE4_BUILD_TESTS, kimaptest doesn't exist.
find_path(KIMAPTEST_INCLUDE_DIR NAMES KF5/kimaptest/fakeserver.h)
find_library(KIMAPTEST_LIBRARY NAMES kimaptest)

MESSAGE( STATUS "KIMAPTEST_INCLUDE_DIR :${KIMAPTEST_INCLUDE_DIR}")
MESSAGE( STATUS "KIMAPTEST_LIBRARY :${KIMAPTEST_LIBRARY}")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

if(KIMAPTEST_INCLUDE_DIR AND KIMAPTEST_LIBRARY)
  MACRO(IMAP_RESOURCE_UNIT_TESTS)
    FOREACH(_testname ${ARGN})
      include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
      add_executable( ${_testname} ${_testname}.cpp dummypasswordrequester.cpp dummyresourcestate.cpp imaptestbase.cpp )
      add_test( ${_testname} ${_testname} )
      ecm_mark_as_test(imap-${_testname})

      target_link_libraries(${_testname} 
            KF5::IMAP Qt5::Gui Qt5::Core ${KIMAPTEST_LIBRARY} Qt5::Test imapresource akonadi-singlefileresource)
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
    testretrievecollectionmetadatatask
    testretrievecollectionstask
    testretrieveitemtask
    testretrieveitemstask
  )

endif()

