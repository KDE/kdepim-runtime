
include_directories(
    ${CMAKE_SOURCE_DIR}/migration
    ${CMAKE_SOURCE_DIR}
)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_migration_agent\")

kde_enable_exceptions()

add_executable(akonadi_migration_agent)
target_sources(akonadi_migration_agent PRIVATE
    migrationagent.cpp
    migrationstatuswidget.cpp
    migrationexecutor.cpp
    migrationscheduler.cpp
    autotests/dummymigrator.cpp
)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(akonadi_migration_agent PROPERTIES UNITY_BUILD ON)
endif()

if( APPLE )
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.migrationagent")
  set_target_properties(akonadi_migration_agent PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi Migrationagent")
endif ()

target_link_libraries(akonadi_migration_agent
    gidmigration
    googlegroupwaremigration
    KF5::AkonadiCore
    KF5::AkonadiAgentBase
    KF5::Contacts
    KF5::WindowSystem
    KF5::JobWidgets
)

install(TARGETS akonadi_migration_agent ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES migrationagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}//akonadi/agents")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
