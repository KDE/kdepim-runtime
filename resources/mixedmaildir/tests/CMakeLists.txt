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
  ${QT_QTTEST_LIBRARY}
)
