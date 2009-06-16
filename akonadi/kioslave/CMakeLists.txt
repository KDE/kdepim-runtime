include_directories( ${kdepim-runtime_SOURCE_DIR} )

set(kio_akonadi_srcs akonadislave.cpp )


kde4_add_plugin(kio_akonadi ${kio_akonadi_srcs})



target_link_libraries(kio_akonadi  ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} )

install(TARGETS kio_akonadi DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

