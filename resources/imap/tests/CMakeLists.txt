set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

MACRO(IMAP_RESOURCE_UNIT_TESTS)
  FOREACH(_testname ${ARGN})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
    kde4_add_unit_test(${_testname} TESTNAME imap-${_testname} NOGUI ${_testname}.cpp
                       dummypasswordrequester.cpp
                       dummyresourcestate.cpp
                       imaptestbase.cpp
                      )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
    target_link_libraries(${_testname} ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY}
                          kimap kimaptest kmime libimapresource)
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
  testmoveitemtask
  testremovecollectiontask
  testremoveitemtask
  testretrievecollectionmetadatatask
  testretrievecollectionstask
  testretrieveitemtask
  testretrieveitemstask
)

set(testsubscriptiondialog_SRCS
  testsubscriptiondialog.cpp
  ../imapaccount.cpp
  ../subscriptiondialog.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
kde4_add_executable(testsubscriptiondialog ${testsubscriptiondialog_SRCS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
target_link_libraries(testsubscriptiondialog ${KDE4_KIO_LIBS} kimap kmime)

