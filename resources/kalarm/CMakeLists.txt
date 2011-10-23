add_subdirectory(kalarm)
add_subdirectory(kalarmdir)

############### Here we install some extra stuff  ###############

## install the MIME type spec file for KAlarm specific MIME types
install(FILES kalarm-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
