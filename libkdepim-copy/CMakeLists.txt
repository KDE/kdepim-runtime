
add_subdirectory( cfgc ) 
add_subdirectory( interfaces ) 
add_subdirectory( tests ) 
add_subdirectory( icons ) 
add_subdirectory( pics ) 
add_subdirectory( kpartsdesignerplugin ) 
add_subdirectory( about ) 

#kde4_add_dcop_skels(libkdepim_SRCS ${CMAKE_SOURCE_DIR}/libkdepim/networkstatus.h )

add_definitions(${QT_QTDBUS_DEFINITIONS})



########### next target ###############

set(kdepim_LIB_SRCS
   ktimeedit.cpp 
   alarmclient.cpp
   kprefsdialog.cpp 
   kpimprefs.cpp 
   categoryselectdialog.cpp 
   categoryeditdialog.cpp 
   kdateedit.cpp 
   kimportdialog.cpp 
   kvcarddrag.cpp 
   addressesdialog.cpp 
   addresseeview.cpp 
   maillistdrag.cpp 
   kconfigpropagator.cpp 
   kconfigwizard.cpp 
   pluginloaderbase.cpp 
   addresseelineedit.cpp 
   kdatepickerpopup.cpp 
   kfileio.cpp 
   configmanager.cpp 
   collectingprocess.cpp 
   ldapclient.cpp 
   overlaywidget.cpp 
   progressmanager.cpp 
   progressdialog.cpp 
   statusbarprogresswidget.cpp 
   ssllabel.cpp 
   completionordereditor.cpp 
   resourceabc.cpp 
   diffalgo.cpp 
   addresseediffalgo.cpp 
   calendardiffalgo.cpp 
   htmldiffalgodisplay.cpp 
   broadcaststatus.cpp 
   kresourceprefs.cpp 
   kabcresourcecached.cpp 
   kxface.cpp 
   kaccount.cpp 
   kaddrbook.cpp 
   kfoldertree.cpp 
   kscoring.cpp 
   kscoringeditor.cpp 
   ksubscription.cpp 
   kwidgetlister.cpp 
   linklocator.cpp 
   qutf7codec.cpp 
   recentaddresses.cpp 
   spellingfilter.cpp 
   addresseeselector.cpp 
   addresseeemailselection.cpp 
   designerfields.cpp 
   kcmdesignerfields.cpp 
   embeddedurlpage.cpp 
   kincidencechooser.cpp 
   groupwarejob.cpp 
   krsqueezedtextlabel.cpp 
   csshelper.cpp 
   distributionlist.cpp 
   kpimurlrequesterdlg.cpp 
   sendsmsdialog.cpp 
   categoryhierarchyreader.cpp 
   autoselectingchecklistitem.cpp 
   improvedlistview.cpp 
   messagestatus.cpp 
   kemailquotinghighter.cpp
   networkstatus.cpp )

kde4_automoc(${kdepim_LIB_SRCS})

kde4_add_ui3_files(kdepim_LIB_SRCS addresspicker.ui )
kde4_add_ui_files(kdepim_LIB_SRCS categoryselectdialog_base.ui categoryeditdialog_base.ui )

#kde4_add_dcop_skels(kdepim_LIB_SRCS 
#	${CMAKE_SOURCE_DIR}/libkdepim/interfaces/MailTransportServiceIface.h 
#	${CMAKE_SOURCE_DIR}/libkdepim/interfaces/AddressBookServiceIface.h
#	addresseelineedit.h )

kde4_add_kcfg_files(kdepim_LIB_SRCS pimemoticons.kcfgc )

kde4_add_library(kdepim SHARED ${kdepim_LIB_SRCS})

target_link_libraries(kdepim  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTDESIGNER_LIBRARY} kresources kabc kcal  kimproxy kspell2 emailfunctions )

set_target_properties(kdepim PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kdepim  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kdepimwidgets_PART_SRCS) 
kde4_add_widget_files(kdepimwidgets_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/kdepim.widgets)

kde4_add_plugin(kdepimwidgets ${kdepimwidgets_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kdepimwidgets )

target_link_libraries(kdepimwidgets  ${KDE4_KIO_LIBS} kdepim )

install(TARGETS kdepimwidgets  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES pimemoticons.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kdepimmacros.h kdepimprotocols.h  DESTINATION ${INCLUDE_INSTALL_DIR} )




#original Makefile.am contents follow:

#SUBDIRS = cfgc interfaces tests icons pics kpartsdesignerplugin about
#
#INCLUDES = -I$(top_srcdir) $(all_includes)
#
#lib_LTLIBRARIES   = libkdepim.la
#libkdepim_la_SOURCES = \
#  ktimeedit.cpp \
#  kprefsdialog.cpp kpimprefs.cpp \
#  categoryselectdialog_base.ui categoryselectdialog.cpp \
#  categoryeditdialog_base.ui categoryeditdialog.cpp \
#  kdateedit.cpp \
#  kimportdialog.cpp kvcarddrag.cpp \
#  addressesdialog.cpp addresspicker.ui addresseeview.cpp \
#  maillistdrag.cpp MailTransportServiceIface.skel \
#  AddressBookServiceIface.skel \
#  kconfigpropagator.cpp kconfigwizard.cpp \
#  pluginloaderbase.cpp addresseelineedit.cpp addresseelineedit.skel \
#  kdatepickerpopup.cpp \
#  kfileio.cpp \
#  configmanager.cpp \
#  collectingprocess.cpp ldapclient.cpp \
#  overlaywidget.cpp progressmanager.cpp progressdialog.cpp \
#  statusbarprogresswidget.cpp ssllabel.cpp completionordereditor.cpp \
#  resourceabc.cpp diffalgo.cpp addresseediffalgo.cpp calendardiffalgo.cpp \
#  htmldiffalgodisplay.cpp broadcaststatus.cpp kresourceprefs.cpp \
#  kabcresourcecached.cpp kxface.cpp \
#  kaccount.cpp kaddrbook.cpp kfoldertree.cpp \
#  kscoring.cpp kscoringeditor.cpp ksubscription.cpp kwidgetlister.cpp \
#  linklocator.cpp qutf7codec.cpp \
#  recentaddresses.cpp spellingfilter.cpp \
#  addresseeselector.cpp addresseeemailselection.cpp \
#  designerfields.cpp kcmdesignerfields.cpp \
#  embeddedurlpage.cpp kincidencechooser.cpp \
#  groupwarejob.cpp pimemoticons.kcfgc \
#  krsqueezedtextlabel.cpp csshelper.cpp distributionlist.cpp \
#  kpimurlrequesterdlg.cpp sendsmsdialog.cpp \
#  categoryhierarchyreader.cpp autoselectingchecklistitem.cpp \
#  improvedlistview.cpp messagestatus.cpp kemailquotinghighter.cpp
#
#MailTransportServiceIface_DCOPIDLNG = true
#MailTransportServiceIface_DIR = $(srcdir)/interfaces
#
#AddressBookServiceIface_DIR = $(srcdir)/interfaces
#
#libkdepim_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 1:0:0
#libkdepim_la_LIBADD  = $(top_builddir)/libkcal/libkcal.la \
#                       $(top_builddir)/libemailfunctions/libemailfunctions.la \
#                       $(LIB_KDE3SUPPORT) $(LIB_KIO) $(LIB_KABC) $(LIB_KPARTS) $(LIB_KIMPROXY) $(LIB_POLL) $(LIB_QTDESIGNER) -lkspell2
##               $top_builddir)/libkdepim/resources/libkpimresources.la
#
#include_HEADERS = kdepimmacros.h kdepimprotocols.h
#
#AM_CXXFLAGS = -DQT_PLUGIN
#kde_widget_LTLIBRARIES = kdepimwidgets.la
#kdepimwidgets_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
#kdepimwidgets_la_LIBADD = $(LIB_KIO) libkdepim.la
#kdepimwidgets_la_SOURCES = kdepimwidgets.cpp 
#
#kdepimwidgets.cpp: $(srcdir)/kdepim.widgets
#	$(MAKEKDEWIDGETS) -o kdepimwidgets.cpp $(srcdir)/kdepim.widgets
#
#kdepimwidgets.lo: kdepimwidgets.moc
#kdepimwidgets.moc: kdepimwidgets.cpp
#	$(MOC) -o kdepimwidgets.moc kdepimwidgets.cpp
#
#CLEANFILES = kdepimwidgets.cpp
#
#kde_kcfg_DATA = pimemoticons.kcfg
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) ../libkpimidentities/*.cpp ../libemailfunctions/*.cpp *.cpp -o $(podir)/libkdepim.pot
#
#include $(top_srcdir)/admin/Doxyfile.am
