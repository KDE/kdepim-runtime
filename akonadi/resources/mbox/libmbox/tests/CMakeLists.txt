set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

########### next target ###############

set(testmbox_SRCS mboxtest.cpp )

kde4_add_unit_test(testmbox TESTNAME mbox-testmbox ${testmbox_SRCS})

target_link_libraries(testmbox ${KDE4_KDECORE_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTTEST_LIBRARY} mbox )

