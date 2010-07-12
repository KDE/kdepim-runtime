kde4_add_plugin(qtwidgetwrappersplugin WITH_PREFIX
  plugin.cpp
  declarativewidgetwrapper.cpp
  qmldateedit.cpp
  qmlcolumnview.cpp
)
target_link_libraries( qtwidgetwrappersplugin messageviewer
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDECLARATIVE_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  akonadi_next
)

install( FILES qmldir DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/qt )
install( TARGETS qtwidgetwrappersplugin DESTINATION ${PLUGIN_INSTALL_DIR}/imports/org/qt )
