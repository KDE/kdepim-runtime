PROJECT(stickynotes)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/libstickynotes/include
  ${CMAKE_BINARY_DIR}/libstickynotes/include 
  ${CMAKE_SOURCE_DIR}/libstickynotes/gui/standalone/include
)

ADD_SUBDIRECTORY(src)

