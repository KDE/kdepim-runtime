add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

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
  ${CMAKE_CURRENT_SOURCE_DIR}/migratorbase.cpp
)


add_subdirectory( kmail )
add_subdirectory( kaddressbook )
add_subdirectory( kjots )
add_subdirectory( gid )
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory( knotes )
  add_subdirectory( kres )
  add_subdirectory( tests )
endif ()

