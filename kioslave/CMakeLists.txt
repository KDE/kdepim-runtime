project(kioslave)
cmake_minimum_required(VERSION 2.8.12)

find_package(ECM 0.0.14 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${resources_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(ECMGenerateHeaders)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

set(KDEPIMLIBS_LIB_VERSION "4.70.0")
set(KF5_VERSION "4.100.0")
set(QT_REQUIRED_VERSION "5.2.0")


find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)

set(kio_akonadi_SRCS akonadislave.cpp )


add_library(kio_akonadi MODULE ${kio_akonadi_SRCS})



target_link_libraries(kio_akonadi  
                      KF5::KDELibs4Support 
                      KF5::ItemModels 
                      KF5::KIOCore 
                      KF5::AkonadiCore 
                      Qt5::DBus
                      Qt5::Widgets
                      )

install(TARGETS kio_akonadi DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

if ("${CMAKE_BINARY_DIR}" STREQUAL "${kioslave_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()


