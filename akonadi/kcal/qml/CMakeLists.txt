kde4_add_plugin( kcalqmlplugin WITH_PREFIX
  plugin.cpp
  kcalitembrowseritem.cpp
)
target_link_libraries(kcalqmlplugin
  ${KDEPIMLIBS_AKONADI_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  ${QT_QTDECLARATIVE_LIBRARY}
  akonadi-kcal_next
  kdepimmobileui
)

install( FILES qmldir DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/kcal )
install( TARGETS kcalqmlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/kde/kcal )

#add_subdirectory(tests)
