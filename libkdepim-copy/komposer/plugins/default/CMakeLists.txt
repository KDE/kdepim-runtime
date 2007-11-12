



########### next target ###############

set(komposer_defaulteditor_PART_SRCS defaulteditor.cpp )


kde4_add_plugin(komposer_defaulteditor ${komposer_defaulteditor_PART_SRCS})



target_link_libraries(komposer_defaulteditor ${KDE4_KPARTS_LIBS} komposer)

install(TARGETS komposer_defaulteditor  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES defaulteditor.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/komposer)
install( FILES defaulteditorui.rc  DESTINATION ${DATA_INSTALL_DIR}/komposer_defaulteditor)
