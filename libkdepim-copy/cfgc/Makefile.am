INCLUDES = -I$(top_srcdir)/libkdepim -I$(top_srcdir) $(all_includes)

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

exampleprefs_base.h: $(top_srcdir)/libkdepim/cfgc/example.cfg \
                $(top_builddir)/libkdepim/cfgc/cfgc
	./cfgc $(top_srcdir)/libkdepim/cfgc/example.cfg

exampleprefs_base.cpp: $(top_srcdir)/libkdepim/cfgc/example.cfg \
                $(top_builddir)/libkdepim/cfgc/cfgc
	./cfgc $(top_srcdir)/libkdepim/cfgc/example.cfg

METASOURCES = AUTO
