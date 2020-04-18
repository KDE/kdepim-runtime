include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)


set(migrationshared_SRCS
  kmigratorbase.cpp
  infodialog.cpp
  entitytreecreatejob.cpp
  migratorbase.cpp
)

ecm_qt_declare_logging_category(migrationshared_SRCS
    HEADER migration_debug.h
    IDENTIFIER MIGRATION_LOG
    CATEGORY_NAME org.kde.pim.migration
    DESCRIPTION "migration (kdepim-runtime)"
    EXPORT KDEPIMRUNTIME
)

add_library(migrationshared STATIC ${migrationshared_SRCS})
target_link_libraries(migrationshared
    KF5::AkonadiCore
    KF5::ConfigCore
    KF5::I18n
    Qt5::Widgets
)


add_subdirectory(gid)

