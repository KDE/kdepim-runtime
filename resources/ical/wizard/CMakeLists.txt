set(ICAL_FILE_DEFAULT_PATH "$HOME/.local/share/korganizer/calendar.ics")

configure_file(icalwizard.es.cmake ${CMAKE_CURRENT_BINARY_DIR}/icalwizard.es)
install ( FILES icalwizard.desktop ${CMAKE_CURRENT_BINARY_DIR}/icalwizard.es icalwizard.ui DESTINATION ${DATA_INSTALL_DIR}/akonadi/accountwizard/ical )
