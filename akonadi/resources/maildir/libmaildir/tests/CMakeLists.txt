set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..   )

########### next target ###############

set(testmaildir_SRCS testmaildir.cpp )


kde4_add_unit_test(testmaildir TESTNAME maildir-testmaildir ${testmaildir_SRCS})

target_link_libraries(testmaildir ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} maildir )

