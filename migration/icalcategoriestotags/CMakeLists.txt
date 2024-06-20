kcfg_generate_dbus_interface(
    ${CMAKE_SOURCE_DIR}/resources/google-groupware/settingsbase.kcfg
    org.kde.Akonadi.Google.Settings
)

add_library(icalcategoriestotagsmigration STATIC)
target_sources(icalcategoriestotagsmigration PRIVATE
    icalcategoriestotagsmigrator.cpp
    icalcategoriestotagsmigrator.h
    ${MIGRATION_AKONADI_SHARED_SOURCES}
    )

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(icalcategoriestotagsmigration PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(icalcategoriestotagsmigration
    KPim6::AkonadiCore
    KF6::CalendarCore
    KF6::I18n
    migrationshared
)

add_subdirectory(autotests)