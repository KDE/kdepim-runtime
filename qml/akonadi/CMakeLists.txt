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
  DESTINATION ${KDE_INSTALL_PLUGINDIR}/imports/org/kde/akonadi
)

add_subdirectory( tests )

endif ()
