kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kpartsdesignerplugin_PART_SRCS kpartsdesignerplugin.cpp )

kde4_automoc(${kpartsdesignerplugin_PART_SRCS})

kde4_add_plugin(kpartsdesignerplugin ${kpartsdesignerplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kpartsdesignerplugin )

target_link_libraries(kpartsdesignerplugin  ${QT_AND_KDECORE_LIBS} kparts )

install_targets(${LIB_INSTALL_DIR}/kde4 kpartsdesignerplugin )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#AM_CXXFLAGS = -DQT_PLUGIN
#METASOURCES = AUTO
#INCLUDES = $(all_includes)
#
#kde_widget_LTLIBRARIES = kpartsdesignerplugin.la
#kpartsdesignerplugin_la_LDFLAGS = $(KDE_PLUGIN) -module $(all_libraries)
#kpartsdesignerplugin_la_LIBADD = $(LIB_KPARTS)
#kpartsdesignerplugin_la_SOURCES = kpartsdesignerplugin.cpp
