INCLUDES = -I$(top_srcdir)/libkdepim/komposer/core $(all_includes)

check_PROGRAMS = testfactory

METASOURCES = AUTO

testfactory_SOURCES = main.cpp managertest.cpp
testfactory_LDFLAGS = $(all_libraries) $(KDE_RPATH)
testfactory_LDADD = ../libkomposer.la

check: testfactory
	@./testfactory 2>&1 | grep "tests:"
