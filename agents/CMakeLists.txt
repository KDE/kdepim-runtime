project(agents)
cmake_minimum_required(VERSION 2.8.12)

find_package(ECM 0.0.14 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${agents_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

set(KF5_VERSION "4.98.0")
set(QT_REQUIRED_VERSION "5.2.0")

find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransport ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimIdentities ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${KF5_VERSION} CONFIG REQUIRED)
# Xsltproc
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")



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
#reactivate it
#add_subdirectory( migration )
add_subdirectory( invitations )

install(FILES akonadinepomukfeederagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")
