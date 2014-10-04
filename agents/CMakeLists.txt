project(agents)
cmake_minimum_required(VERSION 2.8.12)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${agents_SOURCE_DIR}")

  find_package(ECM 1.3.0 CONFIG REQUIRED)
  set(CMAKE_MODULE_PATH ${agents_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

  include(ECMPackageConfigHelpers)
  include(ECMSetupVersion)
  include(FeatureSummary)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDEFrameworkCompilerSettings)

  set(KDEPIMLIBS_LIB_VERSION "4.71.0")
  set(KF5_VERSION "5.3.0")
  set(QT_REQUIRED_VERSION "5.3.0")
  # KF5 package
  find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)

  # KdepimLibs package
  find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5Mime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5AkonadiMime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5MailTransport ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5PimIdentities ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5AkonadiContact ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5CalendarCore ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)


  # Xsltproc
  find_package(Xsltproc)
  set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")

endif()

macro(kdepim_add_agent _target)
    if(KDEPIM_BUILD_AGENTS_AS_PLUGINS)
      add_definitions(-DKDEPIM_PLUGIN_AGENT)
      add_library(${_target} MODULE ${ARGN})
      install( TARGETS ${_target} DESTINATION ${PLUGIN_INSTALL_DIR}/ )
    else()
      add_executable(${_target} ${ARGN})
      install(TARGETS ${_target} ${INSTALL_TARGETS_DEFAULT_ARGS})
    endif()
endmacro()

add_subdirectory( maildispatcher )
add_subdirectory( newmailnotifier )
add_subdirectory( migration )
add_subdirectory( invitations )

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${agents_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

