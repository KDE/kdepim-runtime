set(gid_SRCS
    gidmigrator.cpp
    gidmigrationjob.cpp
)

add_library(gidmigration STATIC ${gid_SRCS})
target_link_libraries(gidmigration
    migrationshared
    KF5::I18n
)

add_executable(gidmigrator main.cpp)
target_link_libraries(gidmigrator
    gidmigration
    KF5::AkonadiWidgets
    KF5::Mime
    )
install(TARGETS gidmigrator ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
