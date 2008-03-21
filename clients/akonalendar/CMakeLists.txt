find_package (KDE4 REQUIRED)
include(KDE4Defaults)

include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${CMAKE_SOURCE_DIR}/akonadi/libakonadi)
include_directories(${CMAKE_SOURCE_DIR}/libkdepim)
include_directories(${KDE4_INCLUDES} )

set(calendar_bin_SRCS
    calendar.cpp
    mainwindow.cpp
    mainwidget.cpp
)


kde4_add_executable(akonalendar_bin ${calendar_bin_SRCS})
set_target_properties(akonalendar_bin PROPERTIES OUTPUT_NAME akonalendar)

target_link_libraries( akonalendar_bin akonadicore kdepim kcalakonadi ${KDE4_KDEUI_LIBRARY} )

install(TARGETS akonalendar_bin DESTINATION ${BIN_INSTALL_DIR} )
