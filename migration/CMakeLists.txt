project(migration)


if ("${CMAKE_SOURCE_DIR}" STREQUAL "${migration_SOURCE_DIR}")
  find_package(ECM 5.9.0 CONFIG REQUIRED)
  set(CMAKE_MODULE_PATH ${migration_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

  include(ECMPackageConfigHelpers)
  include(ECMSetupVersion)
  include(FeatureSummary)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDEFrameworkCompilerSettings)
  set(KDEPIMLIBS_LIB_VERSION "4.75.0")
  set(KF5_VERSION "5.9.0")
  set(QT_REQUIRED_VERSION "5.4.0")

  find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5Mime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5Contacts ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5MailTransport ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5Mbox ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5AkonadiMime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5PimTextEdit ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5Imap ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
endif()

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
#REMOVE IT
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

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


add_subdirectory( gid )

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${migration_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

