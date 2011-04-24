add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DWANT_DEPRECATED_KCAL_API)
add_definitions(-DWANT_DEPRECATED_KABC_API)
add_definitions(-DWANT_DEPRECATED_KRESOURCE_API)
include_directories (${kdepim-runtime_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${QT_QTDBUS_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/shared)

# include (ConfigureChecks.cmake)
# configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( RESOURCE_AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/abstractsubresourcemodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/concurrentjobs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/idarbiterbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/itemfetchadapter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/itemsavejob.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/resourceconfigbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/resourceprivatebase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/storecollectiondialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/storecollectionfilterproxymodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/storecollectionmodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/subresourcebase.cpp
)

add_subdirectory(kabc)
add_subdirectory(kcal)
