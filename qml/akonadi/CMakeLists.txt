if (QT_QTDECLARATIVE_FOUND)
install(FILES
  qmldir
  collectionview.qml
  AkonadiBreadcrumbNavigationView.qml
  CollectionDelegate.qml
  FavoriteSelector.qml
  backgroundtile.png
  border_dot.png
  dividing-line.png
  currentindicator.png
  flapbackground.png
  fuzz.png
  list-line-top.png
  scollable-bottom.png
  scollable-top.png
  selected_bottom.png
  selected_top.png
  sliderbackground.png
  transparentplus.png
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/akonadi
)

add_subdirectory( tests )

endif (QT_QTDECLARATIVE_FOUND)
