
include_directories(
    ${Boost_INCLUDE_DIR}
    ${kdepim-runtime_SOURCE_DIR}/migration
    ${kdepim-runtime_SOURCE_DIR}
    tests
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS} -fPIC")

set(migrationagent_SRCS
    migrationagent.cpp
    migrationstatuswidget.cpp
    migrationexecutor.cpp
    migrationscheduler.cpp
    tests/dummymigrator.cpp
)

add_executable(akonadi_migration_agent ${migrationagent_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.migrationagent")
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Migrationagent")
endif ()

target_link_libraries(akonadi_migration_agent
    gidmigration
    KF5::AkonadiCore
    KF5::AkonadiAgentBase
    KF5::Abc
)

install(TARGETS akonadi_migration_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES migrationagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

add_subdirectory(tests)
