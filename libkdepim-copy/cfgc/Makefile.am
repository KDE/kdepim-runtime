AM_CPPFLAGS = -I$(top_builddir)/libkdepim -I$(top_srcdir) $(all_includes)

check_PROGRAMS = example
#autoexample

example_LDFLAGS = $(all_libraries) $(KDE_RPATH)
example_LDADD = ../libkdepim.la $(LIB_KDECORE)
example_SOURCES = example.cpp exampleprefs_base.kcfgc

#autoexample_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#autoexample_LDADD = ../libkdepim.la $(LIB_KDECORE)
#autoexample_SOURCES = general_base.ui myoptions_base.ui autoexample.cpp

METASOURCES = AUTO
