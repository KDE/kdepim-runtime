if (QT_QTDECLARATIVE_FOUND)
install(FILES
  qmldir
  collectionview.qml
  AkonadiBreadcrumbNavigationView.qml
  CollectionDelegate.qml
  border_dot.png
  check.png
  sliderbackground.png
  transparentplus.png
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/akonadi
)

if(NOT WINCE)
add_subdirectory( tests )
endif(NOT WINCE)

endif (QT_QTDECLARATIVE_FOUND)
