



########### next target ###############

set(komposer_LIB_SRCS 
   plugin.cpp 
   editor.cpp 
   core.cpp 
   attachment.cpp 
   corewidget.cpp 
   pluginmanager.cpp )

kde4_automoc(${komposer_LIB_SRCS})

#kde4_add_dcop_skels(komposer_LIB_SRCS komposerIface.h )

kde4_add_kcfg_files(komposer_LIB_SRCS settings.kcfgc )

kde4_add_library(komposer SHARED ${komposer_LIB_SRCS})

target_link_libraries(komposer  ${KDE4_KPARTS_LIBS} kdepim kutils )

set_target_properties(komposer PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} komposer )


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES komposer.kcfg )
install_files( /include/komposer FILES plugin.h editor.h core.h )
install_files( ${SERVICES_INSTALL_DIR} FILES komposerconfig.desktop   )
install_files( ${DATA_INSTALL_DIR}/komposer FILES komposerui.rc )
install_files( ${SERVICETYPES_INSTALL_DIR} FILES komposerplugin.desktop komposereditor.desktop )




#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_builddir)/libkdepim  $(all_includes)
#METASOURCES = AUTO
#
#lib_LTLIBRARIES = libkomposer.la
#
#libkomposer_la_SOURCES = plugin.cpp editor.cpp core.cpp attachment.cpp corewidget.cpp \
#	settings.kcfgc pluginmanager.cpp komposerIface.skel
#libkomposer_la_LDFLAGS = -no-undefined $(KDE_RPATH) -version-info 1:0:0 $(all_libraries)
#libkomposer_la_LIBADD = $(LIB_KPARTS) $(top_builddir)/libkdepim/libkdepim.la -lkutils
#
##kde_module_LTLIBRARIES = kcm_komposer.la
#
##kcm_komposer_la_SOURCES = prefsmodule.cpp
##kcm_komposer_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
##kcm_komposer_la_LIBADD = libkomposer.la $(top_builddir)/libkdepim/libkdepim.la $(LIB_KDECORE)
#
#kpincludedir = $(includedir)/komposer
#kpinclude_HEADERS = plugin.h editor.h core.h
#
#rcdir = $(kde_datadir)/komposer
#rc_DATA = komposerui.rc
#
#kde_kcfg_DATA = komposer.kcfg
#
#servicetypedir = $(kde_servicetypesdir)
#servicetype_DATA = komposerplugin.desktop komposereditor.desktop
#
#kde_services_DATA = komposerconfig.desktop  
