INCLUDES = $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = libkpimresources.la

libkpimresources_la_SOURCES = plugin.cpp resource.cpp \
             resourcefactory.cpp resourceconfigwidget.cpp \
             resourceconfigdlg.cpp resourceselectdialog.cpp 
libkpimresources_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0 -module
libkpimresources_la_LIBADD = $(LIB_KIO)

kdepimincludedir = $(includedir)/kdepim
kdepiminclude_HEADER = resource.h resourcefactory.h resourceconfigwidget.h \
	resourceconfigdlg.h resourceselectdialog.h 
noinst_HEADERS = plugin.h 
	
