AM_CPPFLAGS = -I$(top_builddir)/libkdepim -I$(top_srcdir)/libemailfunctions $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
LDADD = ../libkdepim.la $(LIB_KDECORE)

check_PROGRAMS = testwizard testaddresseelineedit \
                 testaddresseeselector \
                 testutf7decoder \
                 testutf7encoder \
                 test_kregexp \
                 testdateedit \
                 testlinklocator


testwizard_SOURCES = testwizard.cpp myconfig.kcfgc
testaddresseelineedit_SOURCES = testaddresseelineedit.cpp
testaddresseeselector_SOURCES = testaddresseeselector.cpp
testutf7encoder_SOURCES = testutf7encoder2.cpp
testutf7decoder_SOURCES = testutf7decoder.cpp
test_kregexp_SOURCES = test_kregexp.cpp
testdateedit_SOURCES = testdateedit.cpp
testlinklocator_SOURCES = testlinklocator.cpp

METASOURCES = AUTO

#example_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#example_LDADD = ../libkdepim.la $(LIB_KDECORE)
#example_SOURCES = example.cpp exampleprefs_base.kcfgc

