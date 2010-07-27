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

# test for overwritten methods
kde4_add_unit_test( templatemethodstest
 ../mixedmaildirstore.cpp
 templatemethodstest.cpp
)

target_link_libraries(
  templatemethodstest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for collection creation handling
kde4_add_unit_test( collectioncreatetest
 ../mixedmaildirstore.cpp
 collectioncreatetest.cpp
)

target_link_libraries(
  collectioncreatetest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for collection deletion handling
kde4_add_unit_test( collectiondeletetest
 ../mixedmaildirstore.cpp
 collectiondeletetest.cpp
)

target_link_libraries(
  collectiondeletetest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for collection fetching handling
kde4_add_unit_test( collectionfetchtest
 ../mixedmaildirstore.cpp
 collectionfetchtest.cpp
)

target_link_libraries(
  collectionfetchtest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for collection modification handling
kde4_add_unit_test( collectionmodifytest
 ../mixedmaildirstore.cpp
 collectionmodifytest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  collectionmodifytest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)

# test for collection move handling
kde4_add_unit_test( collectionmovetest
 ../mixedmaildirstore.cpp
 collectionmovetest.cpp
 ${testdata_SRCS}
)

target_link_libraries(
  collectionmovetest
  kmindexreader
  maildir
  mbox
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${QT_QTTEST_LIBRARY}
)
