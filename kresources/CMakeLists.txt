project(akonadi_kres)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# search packages used by KDE
find_package (KDE4 REQUIRED)
find_package (KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

if(WIN32)
    set(LIB_INSTALL_DIR ${LIB_INSTALL_DIR}
                        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
                        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
                        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} )
endif(WIN32)

add_definitions (-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES) # for off_t
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${QT_QTDBUS_INCLUDE_DIR})

# include (ConfigureChecks.cmake)
# configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(kabc)

macro_display_feature_log()
