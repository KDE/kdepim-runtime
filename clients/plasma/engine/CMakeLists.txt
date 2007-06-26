set(akonadi_engine_srcs
    akonadiengine.cpp
)

kde4_automoc(${akonadi_engine_srcs})
kde4_add_plugin(plasma_engine_akonadi ${akonadi_engine_srcs})
target_link_libraries(plasma_engine_akonadi ${KDE4_KDECORE_LIBS} akonadi ${PLASMA_LIBRARIES})

install(TARGETS plasma_engine_akonadi DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-engine-akonadi.desktop DESTINATION ${SERVICES_INSTALL_DIR})

