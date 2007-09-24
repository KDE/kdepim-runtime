set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )




########### next target ###############

set(testfactory_SRCS main.cpp managertest.cpp )




kde4_add_executable(testfactory TEST ${testfactory_SRCS})

target_link_libraries(testfactory  ${KDE4_KDECORE_LIBS} komposer )



########### next target ###############

set(testmanager_SRCS testmanager.cpp )




kde4_add_executable(testmanager TEST ${testmanager_SRCS})

target_link_libraries(testmanager  ${KDE4_KDECORE_LIBS} komposer )



########### next target ###############

set(testkomposer_SRCS testkomposer.cpp )




########### install files ###############





#original Makefile.am contents follow:

#INCLUDES = -I$(top_builddir)/libkdepim/komposer/core $(all_includes)
#
#check_PROGRAMS = testfactory testmanager testkomposer
#
#METASOURCES = AUTO
#
#testfactory_SOURCES = main.cpp managertest.cpp
#testfactory_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#testfactory_LDADD = ../libkomposer.la
#
#testmanager_SOURCES = testmanager.cpp
#testmanager_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#testmanager_LDADD = ../libkomposer.la
#
#testkomposer_SOURCES = testkomposer.cpp
#testkomposer_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#testkomposer_LDADD = ../libkomposer.la
#
#check: testfactory
#	@./testfactory 2>&1 | grep "tests:"
