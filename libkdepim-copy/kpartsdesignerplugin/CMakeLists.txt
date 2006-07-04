



########### next target ###############

set(kpartsdesignerplugin_PART_SRCS kpartsdesignerplugin.cpp )

kde4_automoc(${kpartsdesignerplugin_PART_SRCS})

kde4_add_plugin(kpartsdesignerplugin ${kpartsdesignerplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kpartsdesignerplugin )

target_link_libraries(kpartsdesignerplugin  ${KDE4_KPARTS_LIBS} )

install(TARGETS kpartsdesignerplugin  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############





#original Makefile.am contents follow:

#AM_CXXFLAGS = -DQT_PLUGIN
#METASOURCES = AUTO
#INCLUDES = $(all_includes)
#
#kde_widget_LTLIBRARIES = kpartsdesignerplugin.la
#kpartsdesignerplugin_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
#kpartsdesignerplugin_la_LIBADD = $(LIB_KPARTS)
#kpartsdesignerplugin_la_SOURCES = kpartsdesignerplugin.cpp
