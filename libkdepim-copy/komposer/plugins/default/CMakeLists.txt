



########### next target ###############

set(komposer_defaulteditor_PART_SRCS defaulteditor.cpp )

kde4_automoc(${komposer_defaulteditor_PART_SRCS})

kde4_add_plugin(komposer_defaulteditor WITH_PREFIX ${komposer_defaulteditor_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} komposer_defaulteditor )

target_link_libraries(komposer_defaulteditor ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} komposer)

install(TARGETS komposer_defaulteditor  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES defaulteditor.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/komposer)
install( FILES defaulteditorui.rc  DESTINATION ${DATA_INSTALL_DIR}/komposer_defaulteditor)




#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_builddir)/libkdepim/komposer/core $(all_includes)
#
#kde_module_LTLIBRARIES = libkomposer_defaulteditor.la
#libkomposer_defaulteditor_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)
#libkomposer_defaulteditor_la_LIBADD = ../../core/libkomposer.la $(LIB_KPARTS)
#
#libkomposer_defaulteditor_la_SOURCES = defaulteditor.cpp 
#
#METASOURCES = AUTO
#
#servicedir = $(kde_servicesdir)/komposer
#service_DATA = defaulteditor.desktop
#
#rcdir = $(kde_datadir)/komposer_defaulteditor
#rc_DATA = defaulteditorui.rc
