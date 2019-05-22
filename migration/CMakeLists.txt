include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/
)


set( MIGRATION_AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/kmigratorbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/infodialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/entitytreecreatejob.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/migratorbase.cpp
)


add_subdirectory( gid )

