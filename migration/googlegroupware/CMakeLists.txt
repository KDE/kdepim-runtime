include_directories(${CMAKE_BINARY_DIR}/resources/google-groupware)

set(googlegroupwaremigration_SRCS
    googleresourcemigrator.cpp
    googleresourcemigrator.h
    ${MIGRATION_AKONADI_SHARED_SOURCES}
)

kcfg_generate_dbus_interface(
    ${CMAKE_SOURCE_DIR}/resources/google-groupware/settingsbase.kcfg
    org.kde.Akonadi.Google.Settings
)

qt_add_dbus_interface(googlegroupwaremigration_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Google.Settings.xml
  googlesettingsinterface
)

add_library(googlegroupwaremigration STATIC ${googlegroupwaremigration_SRCS})
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(googlegroupwaremigration PROPERTIES UNITY_BUILD ON)
endif()
target_link_libraries(googlegroupwaremigration
    KPim6::AkonadiCore
    KF6::ConfigGui
    KF6::I18n
    KF6::Wallet
    Qt::DBus
    migrationshared
)
