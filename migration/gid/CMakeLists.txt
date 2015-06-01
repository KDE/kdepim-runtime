set(gid_SRCS
  gidmigrator.cpp
  gidmigrationjob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

add_library(gidmigration STATIC ${gid_SRCS})
target_link_libraries(gidmigration
  KF5::AkonadiCore
  KF5::Mime
  KF5::ConfigCore
  KF5::I18n
  KF5::KDELibs4Support
  Qt5::Core
  Qt5::Widgets
)

add_executable(gidmigrator main.cpp)
target_link_libraries(gidmigrator
  gidmigration
  KF5::AkonadiWidgets
  KF5::Mime
)
install(TARGETS gidmigrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
