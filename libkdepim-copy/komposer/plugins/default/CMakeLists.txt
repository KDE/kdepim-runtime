kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(komposer_defaulteditor_PART_SRCS defaulteditor.cpp )

kde4_automoc(${komposer_defaulteditor_PART_SRCS})

kde4_add_plugin(komposer_defaulteditor WITH_PREFIX ${komposer_defaulteditor_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} komposer_defaulteditor )

target_link_libraries(komposer_defaulteditor  ${QT_AND_KDECORE_LIBS} komposer kparts )

install_targets(${LIB_INSTALL_DIR}/kde4 komposer_defaulteditor )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/komposer FILES defaulteditor.desktop )
install_files( ${DATA_INSTALL_DIR}/komposer_defaulteditor FILES defaulteditorui.rc )

kde4_footer()



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
