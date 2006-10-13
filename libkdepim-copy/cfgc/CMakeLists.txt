



########### next target ###############

set(example_SRCS example.cpp )

kde4_automoc(example ${example_SRCS})

kde4_add_kcfg_files(example example_SRCS exampleprefs_base.kcfgc )

if(KDE4_BUILD_TESTS)

kde4_add_executable(example ${example_SRCS})

target_link_libraries(example  ${KDE4_KDECORE_LIBS} kdepim )

endif(KDE4_BUILD_TESTS)

########### install files ###############





#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_builddir)/libkdepim -I$(top_srcdir) $(all_includes)
#
#check_PROGRAMS = example
##autoexample
#
#example_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#example_LDADD = ../libkdepim.la $(LIB_KDECORE)
#example_SOURCES = example.cpp exampleprefs_base.kcfgc
#
##autoexample_LDFLAGS = $(all_libraries) $(KDE_RPATH)
##autoexample_LDADD = ../libkdepim.la $(LIB_KDECORE)
##autoexample_SOURCES = general_base.ui myoptions_base.ui autoexample.cpp
#
#METASOURCES = AUTO
