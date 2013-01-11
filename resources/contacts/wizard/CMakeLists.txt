
if (WINCE)
  set(CONTACTS_DIRECTORY_DEFAULT_PATH "/My Documents/.local/share/contacts/")
else ()
  set(CONTACTS_DIRECTORY_DEFAULT_PATH "$HOME/.local/share/contacts/")
endif ()

configure_file(contactswizard.es.cmake ${CMAKE_CURRENT_BINARY_DIR}/contactswizard.es)
install ( FILES contactswizard.desktop ${CMAKE_CURRENT_BINARY_DIR}/contactswizard.es contactswizard.ui DESTINATION ${DATA_INSTALL_DIR}/akonadi/accountwizard/contacts/ )
