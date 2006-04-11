AM_CXXFLAGS = -DQT_PLUGIN
METASOURCES = AUTO
INCLUDES = $(all_includes)

kde_widget_LTLIBRARIES = kpartsdesignerplugin.la
kpartsdesignerplugin_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
kpartsdesignerplugin_la_LIBADD = $(LIB_KPARTS) $(LIB_KIO) $(LIB_KDECORE) $(LIB_QT)
kpartsdesignerplugin_la_SOURCES = kpartsdesignerplugin.cpp
