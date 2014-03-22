set(VCARDDIR_FILE_DEFAULT_PATH "$HOME/")

configure_file(vcarddirwizard.es.cmake ${CMAKE_CURRENT_BINARY_DIR}/vcarddirwizard.es)
install ( FILES vcarddirwizard.desktop ${CMAKE_CURRENT_BINARY_DIR}/vcarddirwizard.es vcarddirwizard.ui DESTINATION ${DATA_INSTALL_DIR}/akonadi/accountwizard/vcarddir )
