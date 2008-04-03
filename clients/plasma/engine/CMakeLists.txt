include_directories(
        ${CMAKE_SOURCE_DIR}/akonadi
        ${Boost_INCLUDE_DIR}
)

set(akonadi_engine_srcs
    akonadiengine.cpp
)

kde4_add_plugin(plasma_engine_akonadi ${akonadi_engine_srcs})
target_link_libraries(plasma_engine_akonadi ${KDE4_KDECORE_LIBS} ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${PLASMA_LIBS} ${KDE4_KMIME_LIBS})

install(TARGETS plasma_engine_akonadi DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-engine-akonadi.desktop DESTINATION ${SERVICES_INSTALL_DIR})

