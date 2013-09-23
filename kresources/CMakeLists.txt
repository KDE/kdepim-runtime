add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${kdepim-runtime_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${QT_QTDBUS_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/shared)

add_definitions(
  -DKRESOURCES_DEPRECATED=
  -DKABC_DEPRECATED=
  -DKCAL_DEPRECATED=
)
# This one won't be needed when CMake 2.8.13 is depended on.
add_definitions(
  -DKRESOURCES_DEPRECATED_EXPORT=KRESOURCES_EXPORT
  -DKABC_DEPRECATED_EXPORT=KABC_EXPORT
  -DKCAL_DEPRECATED_EXPORT=KCAL_EXPORT
)

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
