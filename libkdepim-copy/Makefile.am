INCLUDES = $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_SOURCES = \
  kprefs.cpp kprefsdialog.cpp kpimprefs.cpp \
  categoryselectdialog_base.ui categoryselectdialog.cpp \
  categoryeditdialog_base.ui categoryeditdialog.cpp \
  kdateedit.cpp kimportdialog.cpp kvcarddrag.cpp \
  kinputdialog.cpp sidebarextension.cpp addressesdialog.cpp addresspicker.ui \
  interfaces/MailTransportServiceIface.skel

libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0 -module
libkdepim_la_LIBADD  = $(LIB_KIO) $(LIB_KABC) 
#               $top_builddir)/libkdepim/resources/libkpimresources.la 

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/libkdepim.pot

include ../admin/Doxyfile.am
