SUBDIRS = 

INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/libkcal -I$(top_builddir)/libkcal $(all_includes) 

lib_LTLIBRARIES   = libcalendarresources.la
libcalendarresources_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 0:1:0 -module
libcalendarresources_la_LIBADD  = $(LIB_QT) $(top_builddir)/libkcal/libkcal.la \
                     $(top_builddir)/libkdepim/resources/libkderesources.la 

libcalendarresources_la_SOURCES = \
	resourcecalendar.cpp resourcelocal.cpp resourcelocalconfig.cpp \
	multicalendar.cpp

METASOURCES = AUTO

kde_module_LTLIBRARIES = libkcm_kcalendars.la 

libkcm_kcalendars_la_SOURCES = kcmcalendars.cpp
libkcm_kcalendars_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libkcm_kcalendars_la_LIBADD = $(top_builddir)/libkcal/libkcal.la \
        $(top_builddir)/libkdepim/resources/libkderesources.la \
	./libcalendarresources.la

datadir = $(kde_appsdir)/Settings/Components
data_DATA = kcalendars.desktop

linkdir= $(kde_datadir)/resources/calendar
link_DATA = local.desktop

messages:
	$(XGETTEXT) *.cpp -o $(podir)/libkcal.pot

include $(top_srcdir)/admin/Doxyfile.am
