
if(KDE4_BUILD_TESTS)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..   )

########### next target ###############

set(testmaildir_SRCS testmaildir.cpp )

kde4_automoc(${testmaildir_SRCS})

kde4_add_executable(testmaildir NOGUI RUN_UNINSTALLED ${testmaildir_SRCS})

target_link_libraries(testmaildir ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} maildir )

add_test(maildir-testmaildir ${EXECUTABLE_OUTPUT_PATH}/testmaildir)

endif(KDE4_BUILD_TESTS)
