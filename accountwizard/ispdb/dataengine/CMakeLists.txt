project(ispdbengine)



include_directories(
                    ${CMAKE_CURRENT_BINARY_DIR}
   )

set(ispdbengine_srcs
    ispdbengine.cpp
    ../ispdb.cpp
)

kde4_add_plugin(plasma_engine_ispdb ${ispdbengine_srcs})

target_link_libraries(plasma_engine_ispdb
    ${KDE4_KDECORE_LIBS}
    ${KDE4_PLASMA_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS plasma_engine_ispdb DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-engine-ispdb.desktop DESTINATION ${SERVICES_INSTALL_DIR})

