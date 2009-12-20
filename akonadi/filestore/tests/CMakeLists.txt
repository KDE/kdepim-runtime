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
  ${CMAKE_CURRENT_SOURCE_DIR}/../augmentedmbox/
  ${CMAKE_CURRENT_SOURCE_DIR}/../mappedvcard/
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../
)

kde4_add_unit_test( augmentedmboxtest augmentedmboxtest.cpp )
target_link_libraries(
  augmentedmboxtest
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${QT_QTTEST_LIBRARY}
)

kde4_add_unit_test( mappedvcardtest mappedvcardtest.cpp )
target_link_libraries(
  mappedvcardtest
  akonadi-filestore
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${QT_QTTEST_LIBRARY}
)
