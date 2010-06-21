add_definitions(-DTEST_PATH=${CMAKE_CURRENT_SOURCE_DIR})
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${mixedmaildirresource_SOURCE_DIR}/kmindexreader
    ${KDE4_INCLUDES}
)

set( testidxreader_SRCS
    testidxreader.cpp
)

kde4_add_unit_test( testidxreader  ${testidxreader_SRCS} )

target_link_libraries( testidxreader ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} kmindexreader)
