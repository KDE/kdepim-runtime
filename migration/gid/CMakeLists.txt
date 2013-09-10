set(gid_SRCS
  gidmigrator.cpp
  gidmigrationjob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

kde4_add_library(gidmigration ${gid_SRCS})

kde4_add_executable(gidmigrator main.cpp ${gid_SRCS})

target_link_libraries(gidmigrator
  gidmigration
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEPIMLIBS_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS gidmigrator ${INSTALL_TARGETS_DEFAULT_ARGS})
