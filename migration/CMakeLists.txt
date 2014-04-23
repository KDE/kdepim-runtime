add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

kde_enable_exceptions()

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

