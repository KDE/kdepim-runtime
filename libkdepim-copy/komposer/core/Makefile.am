AM_CPPFLAGS = -I$(top_builddir)/libkdepim  $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = libkomposer.la

libkomposer_la_SOURCES = plugin.cpp editor.cpp core.cpp attachment.cpp corewidget.cpp \
	settings.kcfgc pluginmanager.cpp komposerIface.skel
libkomposer_la_LDFLAGS = -no-undefined $(KDE_RPATH) -version-info 1:0:0 $(all_libraries)
libkomposer_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/libkdepim/libkdepim.la -lkutils

#kde_module_LTLIBRARIES = kcm_komposer.la

#kcm_komposer_la_SOURCES = prefsmodule.cpp
#kcm_komposer_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
#kcm_komposer_la_LIBADD = libkomposer.la $(top_builddir)/libkdepim/libkdepim.la $(LIB_KDECORE)

kpincludedir = $(includedir)/komposer
kpinclude_HEADERS = plugin.h editor.h core.h

rcdir = $(kde_datadir)/komposer
rc_DATA = komposerui.rc

kde_kcfg_DATA = komposer.kcfg

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = komposerplugin.desktop komposereditor.desktop

kde_services_DATA = komposerconfig.desktop  
