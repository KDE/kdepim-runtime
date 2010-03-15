if (QT_QTDECLARATIVE_FOUND)
install(FILES
  qmldir
  collectionview.qml
  DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/akonadi
)

add_subdirectory( tests )

endif (QT_QTDECLARATIVE_FOUND)
