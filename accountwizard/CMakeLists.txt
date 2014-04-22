cmake_minimum_required(VERSION 2.8.12)
project(accountwizard)

# ECM setup
find_package(ECM 0.0.12 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

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


# allow to disable GHNS support on resource-constrained systems
option( ACCOUNTWIZARD_NO_GHNS "Disable GHNS support in the account wizard" ${KDEPIM_MOBILE_UI} )
if ( ACCOUNTWIZARD_NO_GHNS )
  add_definitions( -DACCOUNTWIZARD_NO_GHNS )
endif ()

# FIXME: Do we want the same versions in kdepimlibs and kf5?
set(KF5_VERSION "4.98.0")
set(QT_REQUIRED_VERSION "5.2.0")
########### Find packages ###########
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Kross ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransport ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Wallet ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimIdentities ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NewStuff ${KF5_VERSION} CONFIG REQUIRED)
find_package(Qt5UiTools ${QT_REQUIRED_VERSION} CONFIG REQUIRED)

set(accountwizard_srcs
  dialog.cpp
  typepage.cpp
  loadpage.cpp
  global.cpp
  page.cpp
  dynamicpage.cpp
  setupmanager.cpp
  setuppage.cpp
  resource.cpp
  setupobject.cpp
  transport.cpp
  configfile.cpp
  ldap.cpp
  identity.cpp
  servertest.cpp
  personaldatapage.cpp
  ispdb/ispdb.cpp
)

qt5_wrap_ui(accountwizard_srcs
  ui/typepage.ui
  ui/loadpage.ui
  ui/setuppage.ui
  ui/personaldatapage.ui
)

set(accountwizard_libs
  KF5::AkonadiCore
  KF5::MailTransport
  KF5::PimUtils
  KF5::KrossCore
  KF5::KIOCore
  KF5::KMime
  KF5::PimIdentities
  KF5::WidgetsAddons
  KF5::NewStuff
  Qt5::UiTools
)

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  set(accountwizard_srcs ${accountwizard_srcs} providerpage.cpp)
  qt5_wrap_ui(accountwizard_srcs ui/providerpage.ui)
  set(accountwizard_libs ${accountwizard_libs} ${KDE4_KNEWSTUFF3_LIBS})
endif ()

add_executable(accountwizard ${accountwizard_srcs} main.cpp)
target_link_libraries(accountwizard ${accountwizard_libs})

#REACTIVATE IT
#add_library(accountwizard_plugin MODULE ${accountwizard_srcs} inprocess-main.cpp)
#target_link_libraries(accountwizard_plugin ${accountwizard_libs})

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  install(FILES accountwizard.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
endif ()
install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})
#REACTIVATE IT
#install(TARGETS accountwizard_plugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(PROGRAMS accountwizard.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES accountwizard-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
#REACTIVATE it
#update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

add_subdirectory(wizards)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${accountwizard_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

