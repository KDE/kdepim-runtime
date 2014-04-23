set(gid_SRCS
  gidmigrator.cpp
  gidmigrationjob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

add_library(gidmigration STATIC ${gid_SRCS})
target_link_libraries(gidmigration
  
  
  ${KDE4_KDEPIMLIBS_LIBS}
  KF5::AkonadiCore
  KF5::KMime
)

add_executable(gidmigrator main.cpp)
target_link_libraries(gidmigrator
  gidmigration
  
  
  ${KDE4_KDEPIMLIBS_LIBS}
  KF5::AkonadiCore
  KF5::KMime
)
install(TARGETS gidmigrator ${INSTALL_TARGETS_DEFAULT_ARGS})
