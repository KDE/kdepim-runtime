project(migration)
kde_enable_exceptions()

find_package(ECM 0.0.12 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${migration_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

set(KF5_VERSION "4.98.0")
set(QT_REQUIRED_VERSION "5.2.0")

find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/
)

# Xsltproc
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")


set( MIGRATION_AKONADI_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/kmigratorbase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/infodialog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/entitytreecreatejob.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/migratorbase.cpp
)


add_subdirectory( kmail )
add_subdirectory( kaddressbook )
add_subdirectory( kjots )
add_subdirectory( gid )
add_subdirectory( knotes )
