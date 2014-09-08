project(qml_moves)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Not depending on KDE here for easier portability.
find_package(Qt4 REQUIRED)

include_directories(
  ${PROJECT_BINARY_DIR}
)

set(app_SRCS
  dynamictreemodel.cpp
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

add_executable(qml_moves
  ${app_SRCS}
)

target_link_libraries(
  qml_moves
  Qt5::Core
  Qt5::Gui
  Qt5::Declarative
)
