AM_CPPFLAGS = -I../../core $(all_includes)

kde_module_LTLIBRARIES = libkomposer_krichtextplugin.la
libkomposer_krichtextplugin_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
libkomposer_krichtextplugin_la_LIBADD = ../../core/libkomposer.la $(LIB_KPARTS)

libkomposer_krichtextplugin_la_SOURCES = krichtext_plugin.cpp 

METASOURCES = AUTO

servicedir = $(kde_servicesdir)/komposer
service_DATA = krichtextplugin.desktop
