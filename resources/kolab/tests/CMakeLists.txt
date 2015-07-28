set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

# if kdepimlibs was built without -DKDE4_BUILD_TESTS, kimaptest doesn't exist.
find_path(KIMAPTEST_INCLUDE_DIR NAMES kimaptest/fakeserver.h)
find_library(KIMAPTEST_LIBRARY NAMES kimaptest)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

if(KIMAPTEST_INCLUDE_DIR AND KIMAPTEST_LIBRARY)
    MACRO(KOLAB_RESOURCE_ISOLATED_TESTS)
        FOREACH(_testname ${ARGN})
            include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/.. ../../imap/autotests/)
            add_akonadi_isolated_test_advanced(${_testname}.cpp "../../imap/autotests/dummypasswordrequester.cpp;../../imap/autotests/dummyresourcestate.cpp;../../imap/autotests/imaptestbase.cpp" "KF5::IMAP;${KIMAPTEST_LIBRARY};Qt5::Test;imapresource;kolabresource;akonaditest;${Libkolab_LIBRARIES}")
            kde_enable_exceptions()
        ENDFOREACH(_testname)
    ENDMACRO(KOLAB_RESOURCE_ISOLATED_TESTS)

    KOLAB_RESOURCE_ISOLATED_TESTS (
        testretrievetagstask
        testchangeitemstagstask
    )
endif()

