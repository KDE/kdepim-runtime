project(qmlbreadcrumbnavigation)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})


include_directories(
  ${PROJECT_BINARY_DIR}
)

set(app_SRCS
  mainwindow.cpp
  main.cpp
)

set(app_qml_SRCS
  mainview.qml
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
  Qt5::Core
  Qt5::Gui
  Qt5::Declarative
)
