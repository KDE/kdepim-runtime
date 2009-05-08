include_directories(${CMAKE_SOURCE_DIR}/akonadi)
include_directories(${KDE4_INCLUDES})

set(calendar_bin_SRCS
    calendar.cpp
    mainwindow.cpp
    mainwidget.cpp
)


kde4_add_executable(akonalendar_bin ${calendar_bin_SRCS})
set_target_properties(akonalendar_bin PROPERTIES OUTPUT_NAME akonalendar)

target_link_libraries(akonalendar_bin ${KDEPIMLIBS_AKONADI_LIBS} akonadi-kcal ${KDE4_KDEUI_LIBS} ${KDE4_KUTILS_LIBS})

#install(TARGETS akonalendar_bin ${INSTALL_TARGETS_DEFAULT_ARGS})
