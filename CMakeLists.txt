cmake_minimum_required(VERSION 3.5)
set(PIM_VERSION "5.15.40")
set(KDEPIM_RUNTIME_VERSION_NUMBER ${PIM_VERSION})
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
# KDEPIM_DEV_VERSION
# is empty for final versions. For development versions "build" is
# something like "pre", "", "alpha", "beta1", "beta2", "rc1", "rc2".
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

set(KDEPIM_DEV_VERSION alpha)
set(RELEASE_SERVICE_VERSION "20.11.80")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_RUNTIME_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")

configure_file(kdepim-runtime-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepim-runtime-version.h @ONLY)

set(KF5_MIN_VERSION "5.75.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/ ${ECM_MODULE_PATH})


include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(QT_REQUIRED_VERSION "5.13.0")

set(KDEPIMRUNTIME_LIB_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}")
set(KDEPIMRUNTIME_LIB_SOVERSION "5")
set(AKONADI_VERSION "5.15.40")

set(IDENTITYMANAGEMENT_LIB_VERSION "5.15.40")
set(KMAILTRANSPORT_LIB_VERSION "5.15.40")
set(CALENDARUTILS_LIB_VERSION "5.15.40")
set(KIMAP_LIB_VERSION "5.15.41")
set(KMBOX_LIB_VERSION "5.15.40")
set(AKONADICALENDAR_LIB_VERSION "5.15.40")
set(KONTACTINTERFACE_LIB_VERSION "5.15.40")
set(AKONADIKALARM_LIB_VERSION "5.15.40")
set(KMIME_LIB_VERSION "5.15.40")
set(XMLRPCCLIENT_LIB_VERSION "5.15.40")
set(AKONADIMIME_LIB_VERSION "5.15.40")
set(AKONADICONTACT_LIB_VERSION "5.15.40")
set(AKONADINOTE_LIB_VERSION "5.15.40")
set(PIMCOMMON_LIB_VERSION "5.15.40")
set(KGAPI_LIB_VERSION "5.15.41")
set(LIBKDEPIM_LIB_VERSION "5.15.40")
set(KLDAP_LIB_VERSION "5.15.40")
set(GRANTLEETHEME_LIB_VERSION "5.15.40")

set( SharedMimeInfo_MINIMUM_VERSION "1.3" )
find_package(SharedMimeInfo ${SharedMimeInfo_MINIMUM_VERSION} REQUIRED)

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

find_package(Qca-qt5 2.3.0 CONFIG REQUIRED)
set_package_properties(Qca-qt5 PROPERTIES DESCRIPTION "Qt Cryptographic Architecture"
                   URL "https://invent.kde.org/libraries/qca" TYPE REQUIRED
                   PURPOSE "Needed for ews resource.")
option(KDEPIM_RUN_AKONADI_TEST "Enable autotest based on Akonadi." TRUE)
# QT5 package
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED TextToSpeech Network Widgets Test XmlPatterns DBus WebEngineWidgets NetworkAuth)


# KF5 package
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5TextWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED) # for KPluralHandlingSpinBox
find_package(KF5Notifications ${KF5_MIN_VERSION} CONFIG REQUIRED) # pop3, ews
find_package(KF5DocTools ${KF5_MIN_VERSION} CONFIG REQUIRED) # pop3
find_package(KF5Holidays ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DAV ${KF5_MIN_VERSION} CONFIG REQUIRED)

# KdepimLibs package
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${AKONADICONTACT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5AlarmCalendar ${AKONADIKALARM_LIB_VERSION} CONFIG)
find_package(KF5CalendarCore ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IMAP ${KIMAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiNotes ${AKONADINOTE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimCommon ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KPimGAPI ${KGAPI_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5GrantleeTheme ${GRANTLEETHEME_LIB_VERSION} CONFIG REQUIRED)

option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)

#add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054B00)
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")
add_definitions(-DQT_NO_EMIT)

add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(defaultsetup)
add_subdirectory(kioslave)
add_subdirectory(migration)
add_subdirectory(kcms)
add_subdirectory(doc)

## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
ecm_qt_install_logging_categories(
        EXPORT KDEPIMRUNTIME
        FILE kdepim-runtime.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)
