AM_CPPFLAGS = -I$(top_srcdir)/libkdepim -I$(top_srcdir) $(all_includes)

bin_PROGRAMS = cfgc

cfgc_LDFLAGS = $(all_libraries) $(KDE_RPATH)
cfgc_LDADD = $(LIB_KDECORE)
cfgc_SOURCES = cfgc.cpp

check_PROGRAMS = example autoexample

example_LDFLAGS = $(all_libraries) $(KDE_RPATH)
example_LDADD = ../libkdepim.la $(LIB_KDECORE)
example_SOURCES = example.cpp exampleprefs_base.cpp

autoexample_LDFLAGS = $(all_libraries) $(KDE_RPATH)
autoexample_LDADD = ../libkdepim.la $(LIB_KDECORE)
autoexample_SOURCES = general_base.ui myoptions_base.ui autoexample.cpp

example.o exampleprefs_base.o: exampleprefs_base.h
# avoid running the below command in parallel
exampleprefs_base.cpp: exampleprefs_base.h
exampleprefs_base.cpp exampleprefs_base.h: $(srcdir)/example.cfg cfgc
	./cfgc $(srcdir)/example.cfg

METASOURCES = AUTO

CLEANFILES = exampleprefs_base.h exampleprefs_base.cpp

