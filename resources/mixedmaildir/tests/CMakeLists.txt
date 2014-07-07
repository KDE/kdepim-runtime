if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else()
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde_enable_exceptions()

include_directories(
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_BINARY_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# test data
qt5_add_resources( testdata_generated_SRCS testdata.qrc )

set( testdata_SRCS
  ${testdata_generated_SRCS}
  testdatautil.cpp
)


add_executable(testdatatest testdatatest.cpp ${testdata_SRCS})
add_test(testdatatest testdatatest)
ecm_mark_as_test(mixedmaildir-testdatatest)

target_link_libraries(
  testdatatest
  Qt5::Test
  KF5::KDELibs4Support 
)

# put the libraries all tests link against into a variable and use it
# in target_link_libraries instead
set( common_link_libraries
  kmindexreader
  maildir
  akonadi-filestore
  KF5::AkonadiMime
  KF5::AkonadiCore
  KF5::Mbox
  KF5::Mime
  KF5::PimUtils
  Qt5::Test
)

# test for overwritten methods
add_executable(templatemethodstest ../mixedmaildirstore.cpp templatemethodstest.cpp)
add_test(templatemethodstest templatemethodstest)
ecm_mark_as_test(mixedmaildir-templatemethodtest)


target_link_libraries(
  templatemethodstest
  ${common_link_libraries}
)

# test for collection creation handling
kde4_add_unit_test( collectioncreatetest
 TESTNAME mixedmaildir-collectioncreatetest
 ../mixedmaildirstore.cpp
 collectioncreatetest.cpp
)

target_link_libraries(
  collectioncreatetest
  ${common_link_libraries}
)

# test for collection deletion handling
kde4_add_unit_test( collectiondeletetest
 TESTNAME mixedmaildir-collectiondeletetest
 ../mixedmaildirstore.cpp
 collectiondeletetest.cpp
)

target_link_libraries(
  collectiondeletetest
  ${common_link_libraries}
)

# test for collection fetching handling
kde4_add_unit_test( collectionfetchtest
 TESTNAME mixedmaildir-collectionfetchtest
 ../mixedmaildirstore.cpp
 collectionfetchtest.cpp
)

target_link_libraries(
  collectionfetchtest
  ${common_link_libraries}
)

# test for collection modification handling
kde4_add_unit_test( collectionmodifytest
 TESTNAME mixedmaildir-collectionmodifytest
 ../mixedmaildirstore.cpp
 collectionmodifytest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  collectionmodifytest
  ${common_link_libraries}
)

# test for collection move handling
kde4_add_unit_test( collectionmovetest
 TESTNAME mixedmaildir-collectionmovetest
 ../mixedmaildirstore.cpp
 collectionmovetest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  collectionmovetest
  ${common_link_libraries}
)

# test for item creation handling
kde4_add_unit_test( itemcreatetest
 TESTNAME mixedmaildir-itemcreatetest
 ../mixedmaildirstore.cpp
 itemcreatetest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  itemcreatetest
  ${common_link_libraries}
)

# test for item creation handling
kde4_add_unit_test( itemdeletetest
 TESTNAME mixedmaildir-itemdeletetest
 ../mixedmaildirstore.cpp
 itemdeletetest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  itemdeletetest
  ${common_link_libraries}
)

# test for item retrieval handling
kde4_add_unit_test( itemfetchtest
 TESTNAME mixedmaildir-itemfetchtest
 ../mixedmaildirstore.cpp
 itemfetchtest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  itemfetchtest
  ${common_link_libraries}
)

# test for item modification handling
kde4_add_unit_test( itemmodifytest
 TESTNAME mixedmaildir-itemmodifytest
 ../mixedmaildirstore.cpp
 itemmodifytest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  itemmodifytest
  ${common_link_libraries}
)

# test for item move handling
kde4_add_unit_test( itemmovetest
 TESTNAME mixedmaildir-itemmovetest
 ../mixedmaildirstore.cpp
 itemmovetest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  itemmovetest
  ${common_link_libraries}
)

# test for store compact handling
kde4_add_unit_test( storecompacttest
 TESTNAME mixedmaildir-storecompacttest
 ../mixedmaildirstore.cpp
 storecompacttest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  storecompacttest
  ${common_link_libraries}
)
