AM_CPPFLAGS = -I$(top_builddir)/libkdepim/komposer/core $(all_includes)

kde_module_LTLIBRARIES = libkomposer_defaulteditor.la
libkomposer_defaulteditor_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
libkomposer_defaulteditor_la_LIBADD = ../../core/libkomposer.la $(LIB_KPARTS)

libkomposer_defaulteditor_la_SOURCES = defaulteditor.cpp 

METASOURCES = AUTO

servicedir = $(kde_servicesdir)/komposer
service_DATA = defaulteditor.desktop

rcdir = $(kde_datadir)/komposer_defaulteditor
rc_DATA = defaulteditorui.rc
