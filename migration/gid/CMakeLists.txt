set(gid_SRCS
  gidmigrator.cpp
  gidmigrationjob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

add_library(gidmigration STATIC ${gid_SRCS})
target_link_libraries(gidmigration
  KF5::AkonadiCore
  KF5::Mime
  Qt5::Core
)

add_executable(gidmigrator main.cpp)
target_link_libraries(gidmigrator
  gidmigration
  KF5::AkonadiCore
  KF5::Mime
)
install(TARGETS gidmigrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
