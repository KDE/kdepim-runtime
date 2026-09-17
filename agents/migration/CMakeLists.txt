include_directories(
    ${CMAKE_SOURCE_DIR}/migration
    ${CMAKE_SOURCE_DIR}
)

add_definitions(-DTRANSLATION_DOMAIN=\"akonadi_migration_agent\")

kde_enable_exceptions()

add_executable(akonadi_migration_agent)
target_sources(
    akonadi_migration_agent
    PRIVATE
        migrationagent.cpp
        migrationstatuswidget.cpp
        migrationexecutor.cpp
        migrationscheduler.cpp
        migrationagent.h
        migrationstatuswidget.h
        migrationexecutor.h
        migrationscheduler.h
        autotests/dummymigrator.cpp
        autotests/dummymigrator.h
)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        akonadi_migration_agent
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()

if(APPLE)
    embed_info_plist(
	akonadi_migration_agent
	"org.kde.Akonadi.Migration"
	"KDE Akonadi Migration Agent"
    )
endif()

target_link_libraries(
    akonadi_migration_agent
    gidmigration
    icalcategoriestotagsmigration
    KPim6::AkonadiCore
    KPim6::AkonadiAgentWidgetBase
    KF6::Contacts
    KF6::WindowSystem
)

install(
    TARGETS
        akonadi_migration_agent
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
install(FILES migrationagent.desktop DESTINATION "${KDE_INSTALL_DATAROOTDIR}/akonadi/agents")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
