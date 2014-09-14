cmake_minimum_required(VERSION 2.8.12)

project(kdepim-runtime)

############### KDEPIM-Runtime version ################
# KDEPIM_RUNTIME_VERSION
# Version scheme: "x.y.z build".
#
# x is the version number.
# y is the major release number.
# z is the minor release number.
#
# "x.y.z" follow the kdelibs version kdepim is released with.
#
# If "z" is 0, it the version is "x.y"
#
# KDEPIM_RUNTIME_DEV_VERSION
# is empty for final versions. For development versions "build" is
# something like "pre", "alpha1", "alpha2", "beta1", "beta2", "rc1", "rc2".
#
# Examples in chronological order:
#
#    3.0
#    3.0.1
#    3.1 alpha1
#    3.1 beta1
#    3.1 beta2
#    3.1 rc1
#    3.1
#    3.1.1
#    3.2 pre
#    3.2 alpha1

if(NOT DEFINED KDEPIM_RUNTIME_DEV_VERSION)
    set(KDEPIM_RUNTIME_DEV_VERSION "pre")
endif()

set(KDEPIM_RUNTIME_VERSION "4.70${KDEPIM_RUNTIME_DEV_VERSION}")

configure_file(kdepim-runtime-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepim-runtime-version.h @ONLY)


find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})

include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)
include(ECMInstallIcons)

set(KF5_VERSION "5.2.0")
set(QT_REQUIRED_VERSION "5.3.0")

set(KDEPIMLIBS_LIB_VERSION "4.71.0")
set(KDEPIMRUNTIME_LIB_VERSION "4.70.0")
set(KDEPIMRUNTIME_LIB_SOVERSION "5")

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Test Declarative XmlPatterns)

# QT5 package
find_package(Qt5WebKitWidgets ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)



set(KF5AkonadiServer_MIN_VERSION "5.0.40")
find_package(KF5AkonadiServer ${KF5AkonadiServer_MIN_VERSION} CONFIG REQUIRED)

# KF5 package
find_package(KF5KCMUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Kross ${KF5_VERSION} CONFIG REQUIRED)

# KdepimLibs package
find_package(KF5Akonadi ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransport ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Abc ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AlarmCalendar ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimUtils ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mbox ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimTextEdit ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Imap ${KDEPIMLIBS_LIB_VERSION} CONFIG REQUIRED)


#Boost
# Don't use BoostConfig.cmake
set(Boost_NO_BOOST_CMAKE TRUE)
find_package(Boost 1.34.0)
set_package_properties(Boost PROPERTIES DESCRIPTION "Boost C++ Libraries" URL "http://www.boost.org" TYPE REQUIRED PURPOSE "Boost is required by Akonadi")

option(KDEPIM_MOBILE_UI "Build UI for mobile devices instead of for desktops" FALSE)
if (KDEPIM_MOBILE_UI)
  add_definitions( -DKDEPIM_MOBILE_UI )
  if(NOT QT_QTDECLARATIVE_FOUND)
    message(FATAL_ERROR "The QtDeclarative library is required for building the mobile UI")
  endif()
endif ()

option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)


add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(plugins)
add_subdirectory(accountwizard)
add_subdirectory(defaultsetup)
add_subdirectory(resourcetester)
add_subdirectory(kioslave)
add_subdirectory(kcm)
add_subdirectory(migration)
add_subdirectory(qml)


## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})


feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)
