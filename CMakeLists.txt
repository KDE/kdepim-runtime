cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "5.21.40")
set(KDEPIM_RUNTIME_VERSION_NUMBER ${PIM_VERSION})
project(kdepim-runtime VERSION ${KDEPIM_RUNTIME_VERSION_NUMBER})

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
set(RELEASE_SERVICE_VERSION "22.11.40")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_RUNTIME_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")

configure_file(kdepim-runtime-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepim-runtime-version.h @ONLY)

set(KF5_MIN_VERSION "5.99.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/ ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMSetupVersion)
include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(GenerateExportHeader)

set(QT_REQUIRED_VERSION "5.15.2")
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
endif()

set(KDEPIMRUNTIME_LIB_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}")
set(KDEPIMRUNTIME_LIB_SOVERSION "5")
set(AKONADI_VERSION "5.21.40")

set(IDENTITYMANAGEMENT_LIB_VERSION "5.21.40")
set(KMAILTRANSPORT_LIB_VERSION "5.21.40")
set(CALENDARUTILS_LIB_VERSION "5.21.40")
set(KIMAP_LIB_VERSION "5.21.40")
set(KMBOX_LIB_VERSION "5.21.40")
set(AKONADICALENDAR_LIB_VERSION "5.21.40")
set(KONTACTINTERFACE_LIB_VERSION "5.21.40")
set(KMIME_LIB_VERSION "5.21.40")
set(AKONADIMIME_LIB_VERSION "5.21.40")
set(AKONADICONTACT_LIB_VERSION "5.21.40")
set(AKONADINOTE_LIB_VERSION "5.21.40")
set(KGAPI_LIB_VERSION "5.21.40")
set(LIBKDEPIM_LIB_VERSION "5.21.40")
set(KLDAP_LIB_VERSION "5.21.40")

set( SharedMimeInfo_MINIMUM_VERSION "1.3" )
find_package(SharedMimeInfo ${SharedMimeInfo_MINIMUM_VERSION} REQUIRED)

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

find_package(Qca-qt${QT_MAJOR_VERSION} 2.2.0 CONFIG REQUIRED)
set_package_properties(Qca-qt${QT_MAJOR_VERSION} PROPERTIES DESCRIPTION "Qt Cryptographic Architecture"
                   URL "https://invent.kde.org/libraries/qca" TYPE REQUIRED
                   PURPOSE "Needed for ews resource.")
option(KDEPIM_RUN_AKONADI_TEST "Enable autotest based on Akonadi." TRUE)
# QT5 package
find_package(Qt${QT_MAJOR_VERSION}TextToSpeech ${QT_REQUIRED_VERSION} CONFIG)
set_package_properties(Qt${QT_MAJOR_VERSION}TextToSpeech PROPERTIES DESCRIPTION
    "Add support for texttospeed"
    TYPE OPTIONAL
)
if(Qt${QT_MAJOR_VERSION}TextToSpeech_FOUND)
set(HAVE_TEXT_TO_SPEECH_SUPPORT TRUE)
endif()
find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED Network Widgets Test DBus NetworkAuth)
if (QT_MAJOR_VERSION STREQUAL "5" AND BUILD_TESTING)
    find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED XmlPatterns)
endif()
find_package(Qt${QT_MAJOR_VERSION}WebEngineWidgets ${QT_REQUIRED_VERSION} CONFIG)

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
find_package(KF5Holidays ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DAV ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DocTools ${KF5_MIN_VERSION})
set_package_properties(KF5DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)



# KdepimLibs package
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${AKONADICONTACT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IMAP ${KIMAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiNotes ${AKONADINOTE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KPimGAPI ${KGAPI_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)

#it will replace kwallet support
find_package(Qt${QT_MAJOR_VERSION}Keychain CONFIG)
set_package_properties(Qt${QT_MAJOR_VERSION}Keychain PROPERTIES
                                   DESCRIPTION "Provides support for secure credentials storage"
                                   URL "https://github.com/frankosterfeld/qtkeychain"
                                   TYPE REQUIRED)


option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)
ecm_set_disabled_deprecation_versions(QT 5.15.2 KF 5.99.0)


option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(kdepim-keychain_lib qt${QT_MAJOR_VERSION}keychain)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if (USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()


add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(defaultsetup)
add_subdirectory(kioworker)
add_subdirectory(migration)
add_subdirectory(kcms)

## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
ecm_qt_install_logging_categories(
        EXPORT KDEPIMRUNTIME
        FILE kdepim-runtime.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ki18n_install(po)
if(KF5DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

feature_summary(WHAT ALL
                INCLUDE_QUIET_PACKAGES
                FATAL_ON_MISSING_REQUIRED_PACKAGES
)
