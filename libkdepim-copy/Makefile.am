INCLUDES = $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0 -module
libkdepim_la_LIBADD  = $(LIB_KIO)
libkdepim_la_SOURCES = \
  kprefs.cpp kprefsdialog.cpp kpimprefs.cpp \
  categoryselectdialog_base.ui categoryselectdialog.cpp \
  categoryeditdialog_base.ui categoryeditdialog.cpp \
  kdateedit.cpp kimportdialog.cpp

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/libkdepim.pot
