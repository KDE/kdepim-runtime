project(plasmobiff)

set(plasmobiff_SRCS
	plasmobiff.cpp)

kde4_automoc(${plasmobiff_SRCS})
kde4_add_plugin(plasma_applet_plasmobiff ${plasmobiff_SRCS})
target_link_libraries(plasma_applet_plasmobiff plasma ${KDE4_KIO_LIBS})
set_target_properties(plasma_applet_plasmobiff PROPERTIES VERSION 1.0.0 SOVERSION 1)

install(TARGETS plasma_applet_plasmobiff DESTINATION ${PLUGIN_INSTALL_DIR})
