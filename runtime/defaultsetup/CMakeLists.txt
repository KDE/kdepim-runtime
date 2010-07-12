configure_file(defaultaddressbook.cmake ${CMAKE_CURRENT_BINARY_DIR}/defaultaddressbook)
configure_file(defaultcalendar.cmake ${CMAKE_CURRENT_BINARY_DIR}/defaultcalendar)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/defaultcalendar ${CMAKE_CURRENT_BINARY_DIR}/defaultaddressbook DESTINATION ${DATA_INSTALL_DIR}/akonadi/firstrun )
