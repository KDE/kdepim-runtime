AM_CXXFLAGS = -DQT_PLUGIN
METASOURCES = AUTO
INCLUDES = -I$(top_builddir)/libkdepim $(all_includes)

kde_widget_LTLIBRARIES = kpartsdesignerplugin.la
kpartsdesignerplugin_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
kpartsdesignerplugin_la_LIBADD = $(LIB_KPARTS)
kpartsdesignerplugin_la_SOURCES = kpartsdesignerplugin.cpp
