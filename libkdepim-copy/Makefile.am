SUBDIRS = interfaces

INCLUDES = -I$(top_srcdir)/libkdepim $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_SOURCES = \
  kprefs.cpp kprefsdialog.cpp kpimprefs.cpp \
  categoryselectdialog_base.ui categoryselectdialog.cpp \
  categoryeditdialog_base.ui categoryeditdialog.cpp \
  kdateedit.cpp kimportdialog.cpp kvcarddrag.cpp \
  kinputdialog.cpp sidebarextension.cpp infoextension.cpp \
  addressesdialog.cpp addresspicker.ui addresseeview.cpp \
  MailTransportServiceIface.skel AddressBookServiceIface.skel \
  aboutdataextension.cpp

noinst_HEADERS = categoryselectdialog.h categoryeditdialog.h
$(srcdir)/categoryselectdialog.h: categoryselectdialog_base.h
$(srcdir)/categoryeditdialog.h: categoryeditdialog_base.h

MailTransportServiceIface_DIR = $(srcdir)/interfaces
AddressBookServiceIface_DIR = $(srcdir)/interfaces

libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0
libkdepim_la_LIBADD  = $(LIB_KIO) $(LIB_KABC)
#               $top_builddir)/libkdepim/resources/libkpimresources.la

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/libkdepim.pot

include ../admin/Doxyfile.am
