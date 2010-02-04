
set( kjotsmigrator_SRCS
  main.cpp
  kjotsmigrator.cpp
  entitytreecreatejob.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

kde4_add_executable( kjotsmigrator ${kjotsmigrator_SRCS} )

target_link_libraries(kjotsmigrator
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEPIMLIBS_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS kjotsmigrator ${INSTALL_TARGETS_DEFAULT_ARGS})
