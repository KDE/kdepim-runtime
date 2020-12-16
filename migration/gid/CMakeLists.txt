set(gid_SRCS
    gidmigrator.cpp
    gidmigrationjob.cpp
)

add_library(gidmigration STATIC ${gid_SRCS})
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(gidmigration PROPERTIES UNITY_BUILD ON)
endif()

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
