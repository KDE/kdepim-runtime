project(kioslave)
cmake_minimum_required(VERSION 2.8.12)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${kioslave_SOURCE_DIR}")

find_package(ECM 1.1.0 CONFIG REQUIRED)
 set(CMAKE_MODULE_PATH ${resources_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

 include(ECMPackageConfigHelpers)
 include(ECMSetupVersion)
 include(FeatureSummary)
 include(KDEInstallDirs)
 include(KDECMakeSettings)
 include(KDEFrameworkCompilerSettings)

 set(KDEPIMLIBS_LIB_VERSION "4.70.0")
 set(KF5_VERSION "5.1.0")
 set(QT_REQUIRED_VERSION "5.2.0")


 find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
 find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
 find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
endif()

set(kio_akonadi_SRCS akonadislave.cpp akonadislave_debug.cpp)


add_library(kio_akonadi MODULE ${kio_akonadi_SRCS})



target_link_libraries(kio_akonadi  
                      KF5::KIOCore 
                      KF5::AkonadiCore 
                      Qt5::DBus
                      Qt5::Widgets
                      )

install(TARGETS kio_akonadi DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES akonadi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${kioslave_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()


