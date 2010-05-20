project(qmlbreadcrumbnavigation)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Not depending on KDE here for easier portability.
find_package(Qt4 REQUIRED)

include_directories(
  ${QT_INCLUDES}
  ${QT_QTGUI_INCLUDE_DIR}
  ${QT_QTSQL_INCLUDE_DIR}
  ${QT_QTDECLARATIVE_INCLUDE_DIR}
  ${PROJECT_BINARY_DIR}
)

set(app_SRCS
  breadcrumbnavigation.cpp
  breadcrumbnavigationcontext.cpp
  dynamictreemodel.cpp
  dynamictreewidget.cpp
  mainwindow.cpp
  main.cpp
  qtselectionproxymodel.cpp
  kbreadcrumbselectionmodel.cpp
  kmodelindexproxymapper.cpp
  kproxyitemselectionmodel.cpp
)

set(app_qml_SRCS
  mainview.qml
  BreadcrumbNavigationView.qml
  transparentplus.png
  tree_example
  backgroundtile.png
  border_dot.png
  currentindicator.png
  fuzz.png
  selected_bottom.png
  selected_top.png
)

foreach( qml_src ${app_qml_SRCS})
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/${qml_src}" "${CMAKE_BINARY_DIR}/${qml_src}")
endforeach()

qt4_automoc( app_MOC_SRCS ${app_SRCS} )

add_executable(qml_nav
  ${app_SRCS}
)

target_link_libraries(
  qml_nav
  ${QT_QTCORE_LIBRARIES}
  ${QT_QTGUI_LIBRARIES}
  ${QT_QTDECLARATIVE_LIBRARIES}
)
