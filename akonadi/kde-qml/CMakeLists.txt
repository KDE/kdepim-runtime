
kde4_add_plugin( kdeqmlplugin WITH_PREFIX 
  kdeintegration.cpp
  kdeintegrationplugin.cpp 
)
target_link_libraries( kdeqmlplugin ${KDE4_KDEUI_LIBS} QtDeclarative )

install( FILES qmldir DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )
install( TARGETS kdeqmlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )
