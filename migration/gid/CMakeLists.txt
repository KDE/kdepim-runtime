set(gid_SRCS
  gidmigrator.cpp
  gidmigrationjob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

if (ECM_VERSION VERSION_LESS "5.68.0")
    ecm_qt_declare_logging_category(gid_SRCS HEADER migration_debug.h IDENTIFIER MIGRATION_LOG CATEGORY_NAME org.kde.pim.migration)
else()
    ecm_qt_declare_logging_category(gid_SRCS HEADER migration_debug.h IDENTIFIER MIGRATION_LOG CATEGORY_NAME org.kde.pim.migration
        DESCRIPTION "migration (kdepim-runtime)"
        EXPORT KDEPIMRUNTIME
    )
endif()


add_library(gidmigration STATIC ${gid_SRCS})
target_link_libraries(gidmigration
  KF5::AkonadiCore
  KF5::Mime
  KF5::ConfigCore
  KF5::I18n
  Qt5::Core
  Qt5::Widgets
  KF5::WidgetsAddons
)

add_executable(gidmigrator main.cpp)
target_link_libraries(gidmigrator
  gidmigration
  KF5::AkonadiWidgets
  KF5::Mime
)
install(TARGETS gidmigrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
