if (QT_QTDECLARATIVE_FOUND)
install(FILES
  qmldir
  collectionview.qml
  AkonadiBreadcrumbNavigationView.qml
  CollectionDelegate.qml
  FavoriteSelector.qml
  backgroundtile.png
  border_dot.png
  currentindicator.png
  flapbackground.png
  fuzz.png
  selected_bottom.png
  selected_top.png
  sliderbackground.png
  transparentplus.png
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/akonadi
)

add_subdirectory( tests )

endif (QT_QTDECLARATIVE_FOUND)
