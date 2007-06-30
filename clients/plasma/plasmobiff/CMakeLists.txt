project(plasmobiff)

set(plasmobiff_SRCS
	plasmobiff.cpp)

kde4_automoc(${plasmobiff_SRCS})
kde4_add_ui_files(plasmobiff_SRCS plasmobiffConfig.ui )
kde4_add_plugin(plasma_applet_plasmobiff ${plasmobiff_SRCS})
target_link_libraries(plasma_applet_plasmobiff ${PLASMA_LIBRARIES} ${KDE4_KIO_LIBS})
set_target_properties(plasma_applet_plasmobiff PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS plasma_applet_plasmobiff DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-plasmobiff.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES desktoptheme/akonadi.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets)
