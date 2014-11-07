cmake_minimum_required(VERSION 2.8.12)
project(accountwizard)

########### Find packages ###########

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${accountwizard_SOURCE_DIR}")
  find_package(ECM 1.3.0 CONFIG REQUIRED)
  set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

  include(ECMPackageConfigHelpers)
  include(ECMSetupVersion)
  include(FeatureSummary)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDEFrameworkCompilerSettings)

  # FIXME: Do we want the same versions in kdepimlibs and kf5?
  set(KF5_VERSION "5.4.0")
  set(QT_REQUIRED_VERSION "5.3.0")
  set(KDEPIMLIBS_LIB_VERSION "4.74.0")

  # KF5 package
  find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
  find_package(KF5Wallet ${KF5_VERSION} CONFIG REQUIRED)

  # KdepimLibs package
  find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5MailTransport ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5PimUtils ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
  find_package(KF5PimIdentities ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)

endif()

# Qt5 package
find_package(Qt5UiTools ${QT_REQUIRED_VERSION} CONFIG REQUIRED)


find_package(KF5Kross ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NewStuff ${KF5_VERSION} CONFIG REQUIRED)

set( SHARED_MIME_INFO_MINIMUM_VERSION "0.40" )
find_package( SharedMimeInfo REQUIRED )

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

# allow to disable GHNS support on resource-constrained systems
option( ACCOUNTWIZARD_NO_GHNS "Disable GHNS support in the account wizard" ${KDEPIM_MOBILE_UI} )

option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)

# config-enterprise.h is needed for both ENTERPRISE_BUILD and BUILD_EVERYTHING
configure_file(config-enterprise.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-enterprise.h )

if ( ACCOUNTWIZARD_NO_GHNS )
  add_definitions( -DACCOUNTWIZARD_NO_GHNS )
endif ()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${accountwizard_SOURCE_DIR}")
 option(KDEPIM_MOBILE_UI "Build UI for mobile devices instead of for desktops" FALSE)
 if (KDEPIM_MOBILE_UI)
   add_definitions( -DKDEPIM_MOBILE_UI )
   if(NOT QT_QTDECLARATIVE_FOUND)
     message(FATAL_ERROR "The QtDeclarative library is required for building the mobile UI")
   endif()
 endif ()
endif()


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

ki18n_wrap_ui(accountwizard_srcs
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
  KF5::Mime
  KF5::IdentityManagement
  KF5::WidgetsAddons
  KF5::NewStuff
  Qt5::UiTools
  KF5::DBusAddons
)

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  set(accountwizard_srcs ${accountwizard_srcs} providerpage.cpp)
  ki18n_wrap_ui(accountwizard_srcs ui/providerpage.ui)
  set(accountwizard_libs ${accountwizard_libs} KF5::NewStuff)
endif ()

add_executable(accountwizard ${accountwizard_srcs} main.cpp)
target_link_libraries(accountwizard ${accountwizard_libs})

add_library(accountwizard_plugin MODULE ${accountwizard_srcs} inprocess-main.cpp)
target_link_libraries(accountwizard_plugin ${accountwizard_libs})

if ( NOT ACCOUNTWIZARD_NO_GHNS )
  install(FILES accountwizard.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
endif ()
install(TARGETS accountwizard ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS accountwizard_plugin DESTINATION ${PLUGIN_INSTALL_DIR})
install(PROGRAMS org.kde.accountwizard.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES accountwizard-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

add_subdirectory(wizards)
add_subdirectory(ispdb)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${accountwizard_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()

