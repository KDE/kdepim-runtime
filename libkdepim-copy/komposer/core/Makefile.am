AM_CPPFLAGS = -I$(top_srcdir)/libkdepim  $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = libkomposer.la

libkomposer_la_SOURCES = editor.cpp core.cpp prefs.cpp
libkomposer_la_LDFLAGS = -no-undefined $(KDE_RPATH) -version-info 1:0:0 $(all_libraries)
libkomposer_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/libkdepim/libkdepim.la -lkutils

kde_module_LTLIBRARIES = kcm_komposer.la

kcm_komposer_la_SOURCES = prefsmodule.cpp
kcm_komposer_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
kcm_komposer_la_LIBADD = libkomposer.la $(top_builddir)/libkdepim/libkdepim.la $(LIB_KDECORE)

kpincludedir = $(includedir)/komposer
kpinclude_HEADERS = editor.h core.h

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = komposerplugin.desktop

pim_DATA = komposerconfig.desktop
pimdir = $(kde_appsdir)/PIM
