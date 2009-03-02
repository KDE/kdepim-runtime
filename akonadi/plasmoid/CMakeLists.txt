project(kjotsplasmoid)

#########################################################################
# Plasmoid SECTION
#########################################################################

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(kjots_plasmoid_SRCS
    kjots-plasmoid.cpp
)

# Now make sure all files get to the right place
kde4_add_plugin(plasma_applet_kjots ${kjots_plasmoid_SRCS})
target_link_libraries(plasma_applet_kjots
                      kjots_common
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_KIO_LIBS}
)

install(TARGETS plasma_applet_kjots
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kjots-plasmoid.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})


