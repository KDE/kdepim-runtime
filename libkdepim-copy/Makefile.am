SUBDIRS = resources

INCLUDES = $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_SOURCES = \
  kprefs.cpp kprefsdialog.cpp kpimprefs.cpp \
  categoryselectdialog_base.ui categoryselectdialog.cpp \
  categoryeditdialog_base.ui categoryeditdialog.cpp \
  kdateedit.cpp kimportdialog.cpp kvcarddrag.cpp \
  kinputdialog.cpp

libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0 -module
libkdepim_la_LIBADD  = $(LIB_KIO) 
#               $top_builddir)/libkdepim/resources/libkpimresources.la 

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/libkdepim.pot

include $(top_srcdir)/admin/Doxyfile.am
