add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR}/akonadi ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${QT_QTDBUS_INCLUDE_DIR})

# include (ConfigureChecks.cmake)
# configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(kabc)
add_subdirectory(kcal)
