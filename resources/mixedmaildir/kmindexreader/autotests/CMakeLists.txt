add_definitions(-DTEST_PATH=${CMAKE_CURRENT_SOURCE_DIR})

set( testidxreader_SRCS
    testidxreader.cpp
)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)
add_executable( testidxreader  ${testidxreader_SRCS} )
add_test( testidxreader testidxreader )
ecm_mark_as_test(testidxreader)

target_link_libraries( testidxreader  Qt5::Test kmindexreader KF5::AkonadiMime )
