include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)

set(kabcviewer_bin_SRCS
  kabcviewer.cpp
)

kde4_automoc(${kabcviewer_bin_SRCS})

kde4_add_executable(kabcviewer_bin ${kabcviewer_bin_SRCS})
set_target_properties(kabcviewer_bin PROPERTIES OUTPUT_NAME kabcviewer)

target_link_libraries( kabcviewer_bin akonadicomponents kabcakonadi kdepim ${KDE4_KDEUI_LIBRARY} )

install(TARGETS kabcviewer_bin DESTINATION ${BIN_INSTALL_DIR} )

########## next target #######################

set(kabceditor_bin_SRCS
  kabceditor.cpp
)

kde4_automoc(${kabceditor_bin_SRCS})

kde4_add_executable(kabceditor_bin ${kabceditor_bin_SRCS})
set_target_properties(kabceditor_bin PROPERTIES OUTPUT_NAME kabceditor)

target_link_libraries( kabceditor_bin akonadicomponents kabcakonadi kdepim ${KDE4_KDEUI_LIBRARY} )

install(TARGETS kabceditor_bin DESTINATION ${BIN_INSTALL_DIR} )
