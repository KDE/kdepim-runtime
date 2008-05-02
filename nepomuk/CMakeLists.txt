configure_file(nco.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop)
configure_file(nmo.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nmo.desktop)

install(FILES
  nco.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop
  nmo.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nmo.desktop
  DESTINATION
  ${DATA_INSTALL_DIR}/nepomuk/ontologies
)
