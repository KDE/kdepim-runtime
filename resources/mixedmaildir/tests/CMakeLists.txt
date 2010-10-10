if(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH} )
else(${EXECUTABLE_OUTPUT_PATH})
    set( PREVIOUS_EXEC_OUTPUT_PATH . )
endif(${EXECUTABLE_OUTPUT_PATH})
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_BINARY_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# test data
qt4_add_resources( testdata_generated_SRCS testdata.qrc )

set( testdata_SRCS
  ${testdata_generated_SRCS}
  testdatautil.cpp
)

kde4_add_unit_test( testdatatest
 testdatatest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  testdatatest
  ${KDE4_KDECORE_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${QT_QTGUI_LIBRARY}
)

# put the libraries all tests link against into a variable and use it
# in target_link_libraries instead
set( common_link_libraries
  kmindexreader
  maildir
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMBOX_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for overwritten methods
kde4_add_unit_test( templatemethodstest
 ../mixedmaildirstore.cpp
 templatemethodstest.cpp
)

target_link_libraries(
  templatemethodstest
  ${common_link_libraries}
)

# test for collection creation handling
kde4_add_unit_test( collectioncreatetest
 ../mixedmaildirstore.cpp
 collectioncreatetest.cpp
)

target_link_libraries(
  collectioncreatetest
  ${common_link_libraries}
)

# test for collection deletion handling
kde4_add_unit_test( collectiondeletetest
 ../mixedmaildirstore.cpp
 collectiondeletetest.cpp
)

target_link_libraries(
  collectiondeletetest
  ${common_link_libraries}
)

# test for collection fetching handling
kde4_add_unit_test( collectionfetchtest
 ../mixedmaildirstore.cpp
 collectionfetchtest.cpp
)

target_link_libraries(
  collectionfetchtest
  ${common_link_libraries}
)

# test for collection modification handling
kde4_add_unit_test( collectionmodifytest
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
 ../mixedmaildirstore.cpp
 storecompacttest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  storecompacttest
  ${common_link_libraries}
)
