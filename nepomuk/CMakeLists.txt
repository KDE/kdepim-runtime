configure_file(nco.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop)

install(FILES
  nco.trig
  ${CMAKE_CURRENT_BINARY_DIR}/nco.desktop
  DESTINATION
  ${DATA_INSTALL_DIR}/nepomuk/ontologies
)
