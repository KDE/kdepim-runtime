SUBDIRS = cfgc interfaces tests

INCLUDES = -I$(top_srcdir)/libkdepim $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_SOURCES = \
	ktimeedit.cpp \
  kprefsdialog.cpp kpimprefs.cpp \
  categoryselectdialog_base.ui categoryselectdialog.cpp \
  categoryeditdialog_base.ui categoryeditdialog.cpp \
  kdateedit.cpp kimportdialog.cpp kvcarddrag.cpp \
  sidebarextension.cpp infoextension.cpp \
  addressesdialog.cpp addresspicker.ui addresseeview.cpp \
  maillistdrag.cpp MailTransportServiceIface.skel \
  AddressBookServiceIface.skel \
  kconfigpropagator.cpp kconfigwizard.cpp \
  weaver.cpp weaverextensions.cpp weaverlogger.cpp \
  pluginloaderbase.cpp addresseelineedit.cpp \
  klistviewsearchline_pimcopy.cpp kdatepickerpopup.cpp \
  clicklineedit.cpp \
  email.cpp kfileio.cpp processcollector.cpp configmanager.cpp \
  identitymanager.cpp identity.cpp identitymanager.skel \
  identitycombo.cpp

noinst_HEADERS = categoryselectdialog.h categoryeditdialog.h ktimeedit.h 
$(srcdir)/categoryselectdialog.h: categoryselectdialog_base.h
$(srcdir)/categoryeditdialog.h: categoryeditdialog_base.h

MailTransportServiceIface_DCOPIDLNG = true
MailTransportServiceIface_DIR = $(srcdir)/interfaces

AddressBookServiceIface_DIR = $(srcdir)/interfaces

libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0
libkdepim_la_LIBADD  = $(LIB_KIO) $(LIB_KABC) $(LIB_KDEUI)
#               $top_builddir)/libkdepim/resources/libkpimresources.la

identitymanager_DCOPIDLNG = true

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/libkdepim.pot

include ../admin/Doxyfile.am
