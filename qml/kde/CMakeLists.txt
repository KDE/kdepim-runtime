if (QT_QTDECLARATIVE_FOUND)

if (KDEQMLPLUGIN_STATIC)
  set(LIBRARY_TYPE "STATIC")
else()
  set(LIBRARY_TYPE)
endif ()

add_library(kdeqmlplugin MODULE ${LIBRARY_TYPE} WITH_PREFIX
  kdeintegration.cpp
  kdeintegrationplugin.cpp
)
target_link_libraries(kdeqmlplugin  Qt5::Declarative ${QT_QTSCRIPT_LIBRARY})

if (KDEQMLPLUGIN_STATIC)
  install( FILES qmldir_without_kdeqmlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde RENAME qmldir)
else()
  install( FILES qmldir DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )
endif ()

install( FILES
  BreadcrumbNavigationView.qml
  SlideoutPanel.qml
  SlideoutPanelContainer.qml
  Dialog.qml
  Flap.qml
  Flap2.qml
  dividing-line.png
  dividing-line-horizontal.png
  list-line-top.png
  scrollable-bottom.png
  scrollable-top.png
  flap-collapsed-top.png
  flap-collapsed-mid.png
  flap-collapsed-bottom.png
  flap-expanded-top.png
  flap-expanded-mid.png
  flap-expanded-bottom.png
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )

install( TARGETS kdeqmlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde )

#add_subdirectory(tests)

endif ()
