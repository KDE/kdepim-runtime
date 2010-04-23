if (QT_QTDECLARATIVE_FOUND)

kde4_add_plugin( kdeqmlplugin WITH_PREFIX
  kdeintegration.cpp
  kdeintegrationplugin.cpp
)
target_link_libraries(kdeqmlplugin ${KDE4_KDEUI_LIBS} ${QT_QTDECLARATIVE_LIBRARY})

install( FILES qmldir
  BreadcrumbNavigationView.qml
  SlideoutPanel.qml
  SlideoutPanelContainer.qml
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )

install( TARGETS kdeqmlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )

#add_subdirectory(tests)

endif (QT_QTDECLARATIVE_FOUND)
