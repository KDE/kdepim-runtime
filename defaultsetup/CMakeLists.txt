configure_file(defaultaddressbook.desktop ${CMAKE_CURRENT_BINARY_DIR}/defaultaddressbook)
configure_file(defaultcalendar.desktop ${CMAKE_CURRENT_BINARY_DIR}/defaultcalendar)
configure_file(defaultnotebook.desktop ${CMAKE_CURRENT_BINARY_DIR}/defaultnotebook)

install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/defaultcalendar
                ${CMAKE_CURRENT_BINARY_DIR}/defaultaddressbook
                ${CMAKE_CURRENT_BINARY_DIR}/defaultnotebook
                DESTINATION ${DATA_INSTALL_DIR}/akonadi/firstrun )
