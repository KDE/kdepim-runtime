
if (WINCE)
  set(ICAL_FILE_DEFAULT_PATH "/My Documents/.local/share/korganizer/calendar.ics")
else (WINCE)
  set(ICAL_FILE_DEFAULT_PATH "$HOME/.local/share/korganizer/calendar.ics")
endif (WINCE)

configure_file(icalwizard.es.cmake ${CMAKE_CURRENT_BINARY_DIR}/icalwizard.es)
install ( FILES icalwizard.desktop ${CMAKE_CURRENT_BINARY_DIR}/icalwizard.es icalwizard.ui DESTINATION ${DATA_INSTALL_DIR}/akonadi/accountwizard/ical )
