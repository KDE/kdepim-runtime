SUBDIRS = cfgc interfaces tests pics

INCLUDES = -I$(top_srcdir) $(all_includes)

lib_LTLIBRARIES   = libkdepim.la
libkdepim_la_SOURCES = \
  ktimeedit.cpp part.cpp \
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
  kdatepickerpopup.cpp \
  clicklineedit.cpp \
  email.cpp kfileio.cpp configmanager.cpp \
  collectingprocess.cpp ldapclient.cpp \
  kimproxy.cpp kimiface.stub kimproxyiface.skel \
  overlaywidget.cpp progressmanager.cpp progressdialog.cpp \
  statusbarprogresswidget.cpp ssllabel.cpp completionordereditor.cpp \
  resourceabc.cpp diffalgo.cpp addresseediffalgo.cpp calendardiffalgo.cpp \
  htmldiffalgodisplay.cpp broadcaststatus.cpp kresourceprefs.cpp \
  kpixmapregionselectorwidget.cpp kpixmapregionselectordialog.cpp \
  kabcresourcecached.cpp

noinst_HEADERS = categoryselectdialog.h categoryeditdialog.h ktimeedit.h \
  kpixmapregionselectorwidget.h kpixmapregionselectordialog.h
$(srcdir)/categoryselectdialog.h: categoryselectdialog_base.h
$(srcdir)/categoryeditdialog.h: categoryeditdialog_base.h kimproxyiface.h kimiface.h

MailTransportServiceIface_DCOPIDLNG = true
MailTransportServiceIface_DIR = $(srcdir)/interfaces

AddressBookServiceIface_DIR = $(srcdir)/interfaces

libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0
libkdepim_la_LIBADD  = $(LIB_KIO) $(LIB_KABC) $(LIB_KPARTS) \
  $(top_builddir)/libkcal/libkcal.la $(LIB_KDEUI) 
#               $top_builddir)/libkdepim/resources/libkpimresources.la



AM_CXXFLAGS = -DQT_PLUGIN
kde_widget_LTLIBRARIES = kdepimwidgets.la
kdepimwidgets_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
kdepimwidgets_la_LIBADD = $(LIB_KIO) libkdepim.la
kdepimwidgets_la_SOURCES = kdepimwidgets.cpp 

kdepimwidgets.cpp: kdepim.widgets
	makekdewidgets -o kdepimwidgets.cpp $<

CLEANFILES = kdepimwidgets.cpp



METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) ../libkpimidentities/*.cpp *.cpp -o $(podir)/libkdepim.pot

include ../admin/Doxyfile.am
