add_definitions(-DWANT_DEPRECATED_KCAL_API)
add_definitions(-DWANT_DEPRECATED_KABC_API)
add_definitions(-DWANT_DEPRECATED_KRESOURCE_API)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( MIGRATION_AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/kmigratorbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/infodialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/entitytreecreatejob.cpp
)


add_subdirectory( akregator )
add_subdirectory( kmail )
add_subdirectory( kaddressbook )
add_subdirectory( kjots )
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory( kres )
  add_subdirectory( tests )
endif (KDEPIMLIBS_KRESOURCES_LIBS)

