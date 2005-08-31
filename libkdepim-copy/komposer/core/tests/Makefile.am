INCLUDES = -I$(top_builddir)/libkdepim/komposer/core $(all_includes)

check_PROGRAMS = testfactory testmanager testkomposer

METASOURCES = AUTO

testfactory_SOURCES = main.cpp managertest.cpp
testfactory_LDFLAGS = $(all_libraries) $(KDE_RPATH)
testfactory_LDADD = ../libkomposer.la

testmanager_SOURCES = testmanager.cpp
testmanager_LDFLAGS = $(all_libraries) $(KDE_RPATH)
testmanager_LDADD = ../libkomposer.la

testkomposer_SOURCES = testkomposer.cpp
testkomposer_LDFLAGS = $(all_libraries) $(KDE_RPATH)
testkomposer_LDADD = ../libkomposer.la

check: testfactory
	@./testfactory 2>&1 | grep "tests:"
