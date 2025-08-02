cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "6.5.40")
set(KDEPIM_RUNTIME_VERSION_NUMBER ${PIM_VERSION})
project(kdepim-runtime VERSION ${KDEPIM_RUNTIME_VERSION_NUMBER})

set(KDEPIM_DEV_VERSION alpha)
set(RELEASE_SERVICE_VERSION "25.11.70")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_RUNTIME_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")

configure_file(kdepim-runtime-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/kdepim-runtime-version.h @ONLY)

set(KF_MIN_VERSION "6.15.0")

find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/ ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMSetupVersion)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)
include(GenerateExportHeader)

set(QT_REQUIRED_VERSION "6.8.0")

set(KDEPIMRUNTIME_LIB_VERSION "${KDEPIM_RUNTIME_VERSION_NUMBER}")
set(KDEPIMRUNTIME_LIB_SOVERSION "6")
set(AKONADI_VERSION "6.5.40")

set(IDENTITYMANAGEMENT_LIB_VERSION "6.5.40")
set(KMAILTRANSPORT_LIB_VERSION "6.5.40")
set(CALENDARUTILS_LIB_VERSION "6.5.40")
set(KIMAP_LIB_VERSION "6.5.40")
set(KMBOX_LIB_VERSION "6.5.40")
set(AKONADICALENDAR_LIB_VERSION "6.5.40")
set(KMIME_LIB_VERSION "6.5.40")
set(AKONADIMIME_LIB_VERSION "6.5.40")
set(AKONADICONTACT_LIB_VERSION "6.5.40")
set(KGAPI_LIB_VERSION "6.5.40")
set(KLDAP_LIB_VERSION "6.5.40")
set(KTEXTADDONS_MIN_VERSION "1.6.0")
set(QT6KEYCHAIN_LIB_VERSION "0.14.2")
set(PIMCOMMON_LIB_VERSION "6.5.40")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SharedMimeInfo_MINIMUM_VERSION "1.10")
find_package(SharedMimeInfo ${SharedMimeInfo_MINIMUM_VERSION} REQUIRED)

find_package(Sasl2)
set_package_properties(Sasl2 PROPERTIES TYPE REQUIRED)

find_package(Qca-qt6 2.3.7 CONFIG REQUIRED)
set_package_properties(Qca-qt6 PROPERTIES DESCRIPTION "Qt Cryptographic Architecture"
                   URL "https://invent.kde.org/libraries/qca" TYPE REQUIRED
                   PURPOSE "Needed for ews resource.")
option(KDEPIM_RUN_AKONADI_TEST "Enable autotest based on Akonadi." TRUE)
# QT package
find_package(Qt6TextToSpeech ${QT_REQUIRED_VERSION} CONFIG)
set_package_properties(Qt6TextToSpeech PROPERTIES DESCRIPTION
    "Add support for texttospeech"
    TYPE OPTIONAL
)
if(Qt6TextToSpeech_FOUND)
set(HAVE_TEXT_TO_SPEECH_SUPPORT TRUE)
endif()
find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Network Widgets Test DBus NetworkAuth Concurrent)
find_package(Qt6WebEngineWidgets ${QT_REQUIRED_VERSION} CONFIG)

# KF6 package
find_package(KF6Config ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ConfigWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6NotifyConfig ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6KIO ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ItemModels ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Codecs ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WindowSystem ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Notifications ${KF_MIN_VERSION} CONFIG REQUIRED) # pop3, ews
find_package(KF6KCMUtils ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6DAV ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Wallet ${KF_MIN_VERSION} CONFIG REQUIRED)

find_package(KF6TextCustomEditor ${KTEXTADDONS_MIN_VERSION} CONFIG REQUIRED)

# KdepimLibs package
find_package(KPim6Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KPim6Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MailTransport ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6IdentityManagementWidgets ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiContactWidgets ${AKONADICONTACT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF6Contacts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KPim6CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6IMAP ${KIMAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiCalendarCore ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6GAPI ${KGAPI_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6LdapWidgets ${KLDAP_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim6PimCommonActivities ${PIMCOMMON_LIB_VERSION} CONFIG)
find_package(KPim6PimCommonAkonadi ${PIMCOMMON_LIB_VERSION} CONFIG)

find_package(Qt6Keychain ${QT6KEYCHAIN_LIB_VERSION} CONFIG)
set_package_properties(Qt6Keychain PROPERTIES
                                   DESCRIPTION "Provides support for secure credentials storage"
                                   URL "https://github.com/frankosterfeld/qtkeychain"
                                   TYPE REQUIRED)

option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)
ecm_set_disabled_deprecation_versions(QT 6.7.2 KF 6.17.0)


option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)
add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
    add_definitions(-DCOMPILE_WITH_UNITY_CMAKE_SUPPORT)
endif()

if(TARGET KPim6::PimCommonActivities)
	option(OPTION_USE_PLASMA_ACTIVITIES "Activate plasma activities" ON)
    if(OPTION_USE_PLASMA_ACTIVITIES)
        if(NOT WIN32 AND NOT APPLE)
            set(OPTION_USE_PLASMA_ACTIVITIES true)
        else()
            set(OPTION_USE_PLASMA_ACTIVITIES false)
        endif()
    endif()
    if(OPTION_USE_PLASMA_ACTIVITIES)
        if(NOT WIN32 AND NOT APPLE)
            find_package(PlasmaActivities "6.3.0")
            set_package_properties(PlasmaActivities PROPERTIES DESCRIPTION
                "Allow to compile with plasma activities"
                TYPE OPTIONAL
            )
        endif()

        if(TARGET Plasma::Activities)
            set(HAVE_ACTIVITY_SUPPORT TRUE)
        endif()
    endif()
endif()

if(BUILD_TESTING)
   add_definitions(-DBUILD_TESTING)
endif()
include_directories(${kdepim-runtime_SOURCE_DIR} ${kdepim-runtime_BINARY_DIR})
configure_file(config-kdepim-runtime.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-kdepim-runtime.h)

add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(defaultsetup)
add_subdirectory(kioworker)
add_subdirectory(migration)
add_subdirectory(kcms)
add_subdirectory(kmailtransport)
add_subdirectory(images)
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

ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
