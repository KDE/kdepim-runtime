cmake_minimum_required(VERSION 3.0)
set(KDEPIM_RUNTIME_VERSION_NUMBER "5.7.40")
project(kdepim-runtime VERSION ${KDEPIM_RUNTIME_VERSION_NUMBER})

if (POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()

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
# something like "pre", "", "alpha2", "beta1", "beta2", "rc1", "rc2".
#
# Examples in chronological order:
#
#    3.0
#    3.0.1
#    3.1 
#    3.1 beta1
#    3.1 beta2
#    3.1 rc1
#    3.1
#    3.1.1
#    3.2 pre
#    3.2 

if(NOT DEFINED KDEPIM_RUNTIME_DEV_VERSION)
    set(KDEPIM_RUNTIME_DEV_VERSION "rc")
endif()

set(KDEPIM_RUNTIME_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}${KDEPIM_RUNTIME_DEV_VERSION}")

configure_file(kdepim-runtime-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepim-runtime-version.h @ONLY)

set(KF5_VERSION "5.39.0")

find_package(ECM ${KF5_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/ ${ECM_MODULE_PATH})

# TODO: Remove the cmake/deprecated directory when we'll require ECM >= 5.41.0
if("${ECM_VERSION}" VERSION_LESS "5.41.0")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${kdepim-runtime_SOURCE_DIR}/cmake/deprecated)
endif()

include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMCoverageOption)

set(QT_REQUIRED_VERSION "5.8.0")

set(KDEPIMRUNTIME_LIB_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}")
set(KDEPIMRUNTIME_LIB_SOVERSION "5")
set(AKONADI_VERSION "5.6.90")

set(KCONTACTS_LIB_VERSION "5.6.90")
set(KCALENDARCORE_LIB_VERSION "5.6.90")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.6.90")
set(KMAILTRANSPORT_LIB_VERSION "5.6.90")
set(CALENDARUTILS_LIB_VERSION "5.6.90")
set(KDAV_LIB_VERSION "5.6.90")
set(KIMAP_LIB_VERSION "5.6.90")
set(KMBOX_LIB_VERSION "5.6.90")
set(AKONADICALENDAR_LIB_VERSION "5.6.90")
set(KONTACTINTERFACE_LIB_VERSION "5.6.90")
set(AKONADIKALARM_LIB_VERSION "5.6.90")
set(KMIME_LIB_VERSION "5.6.90")
set(XMLRPCCLIENT_LIB_VERSION "5.6.90")
set(KCONTACTS_LIB_VERSION "5.6.90")
set(AKONADIMIME_LIB_VERSION "5.6.90")
set(AKONADICONTACT_LIB_VERSION "5.6.90")
set(AKONADINOTE_LIB_VERSION "5.6.90")
set(PIMCOMMON_LIB_VERSION_LIB "5.6.90")
set(KGAPI_LIB_VERSION "5.6.90")

set( SHARED_MIME_INFO_MINIMUM_VERSION "1.0" )
find_package( SharedMimeInfo REQUIRED )

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

# QT5 package
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Network Widgets Test XmlPatterns DBus WebEngineWidgets)
find_package(Qt5 OPTIONAL_COMPONENTS TextToSpeech)
if (NOT Qt5TextToSpeech_FOUND)
    message(STATUS "Qt5TextToSpeech not found, speech feature will be disabled")
else()
    add_definitions(-DHAVE_TEXTTOSPEECH)
endif()


# KF5 package
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5TextWidgets ${KF5_VERSION} CONFIG REQUIRED) # for KPluralHandlingSpinBox
find_package(KF5Notifications ${KF5_VERSION} CONFIG REQUIRED) # pop3

# KdepimLibs package
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KPimKDAV ${KDAV_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${AKONADICONTACT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KCONTACTS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AlarmCalendar ${AKONADIKALARM_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IMAP ${KIMAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiNotes ${AKONADINOTE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimCommon ${PIMCOMMON_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KPimGAPI ${KGAPI_LIB_VERSION} CONFIG REQUIRED)

option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
add_definitions( -DQT_NO_URL_CAST_FROM_STRING )
add_definitions( -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT )
#add_definitions( -DQT_DISABLE_DEPRECATED_BEFORE=0x060000 )


# Extra package
find_package(KPimGAPI "5.6.2" CONFIG)

add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(plugins)
add_subdirectory(defaultsetup)
add_subdirectory(kioslave)
add_subdirectory(migration)
add_subdirectory(doc)

## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})


install( FILES kdepim-runtime.renamecategories kdepim-runtime.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)
