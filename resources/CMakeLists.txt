project(resources)
cmake_minimum_required(VERSION 2.8.12)

find_package(ECM 0.0.13 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${resources_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

set(KF5_VERSION "4.98.0")
set(QT_REQUIRED_VERSION "5.2.0")

set(KDEPIM_LIB_VERSION "4.70.0")
set(KDEPIM_LIB_SOVERSION "5")


find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KDELibs4Support ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Akonadi ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Abc ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5PimIdentities ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransport ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KF5_VERSION} CONFIG REQUIRED)


# Xsltproc
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")

find_package(AccountsQt 1.11 QUIET CONFIG)
set_package_properties(AccountsQt PROPERTIES DESCRIPTION "Qt bindings for accounts-sso" URL "https://code.google.com/p/accounts-sso/" TYPE OPTIONAL PURPOSE "Required to support AccountsQt in different resources")

find_package(SignOnQt 8.56 QUIET CONFIG)
set_package_properties(SignOnQt PROPERTIES DESCRIPTION "SignOn Qt bindings for accounts-sso" URL "https://code.google.com/p/accounts-sso/" TYPE OPTIONAL PURPOSE "Required to support SignOn in different resources")

# Libkolab
find_package(Libkolab 0.5 QUIET CONFIG)
set_package_properties(Libkolab PROPERTIES DESCRIPTION "libkolab" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab Format libraries are required to build the Kolab Groupware Resource")

# Libkolabxml
find_package(Libkolabxml 1.0 QUIET CONFIG)
set_package_properties(Libkolabxml PROPERTIES DESCRIPTION "Kolabxml" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab XML Format Schema Definitions Library is required to build the Kolab Groupware Resource")

# Libkgapi2
find_package(LibKGAPI2 1.9.81 QUIET CONFIG)
set_package_properties(LibKGAPI2 PROPERTIES DESCRIPTION "KDE-based library for accessing various Google services" URL "https://projects.kde.org/libkgapi" TYPE OPTIONAL PURPOSE "LibKGAPI is required to build Akonadi resources to access Google Contacts, Calendars and Tasks")

# Libkfbapi
find_package(LibKFbAPI 1.0 QUIET CONFIG)
set_package_properties(LibKFbAPI PROPERTIES DESCRIPTION "A library to access Facebook services" URL "http://projects.kde.org/libkfbapi" TYPE OPTIONAL PURPOSE "LibKFbAPI is required to build Akonadi resources to access Facebook's contacts, events, notes and posts" )

if(LibKGAPI2_FOUND OR LibKFbAPI_FOUND)
  find_package(QJSON)
  set_package_properties(QJSON PROPERTIES DESCRIPTION "Qt library for handling JSON data" URL "http://qjson.sourceforge.net/" TYPE REQUIRED PURPOSE "Required to build the Google and Facebook resources")
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/shared
  ${CMAKE_CURRENT_BINARY_DIR}/shared
  ${CMAKE_CURRENT_SOURCE_DIR}/folderarchivesettings/
)



#add_definitions( -DQT_NO_CAST_FROM_ASCII )
#add_definitions( -DQT_NO_CAST_TO_ASCII )

set( AKONADI_SINGLEFILERESOURCE_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourcebase.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialogbase.cpp
)

if (KDEPIM_MOBILE_UI)
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog_mobile.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
else ()
set( AKONADI_SINGLEFILERESOURCE_SHARED_UI
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog_desktop.ui
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/singlefileresourceconfigdialog.ui
)
endif ()

set( AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionannotationsattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/collectionflagsattribute.cpp
)

set( AKONADI_IMAPATTRIBUTES_SHARED_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapaclattribute.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/shared/imapquotaattribute.cpp
)

#add_subdirectory( akonotes )
#add_subdirectory( kalarm )
#add_subdirectory( contacts )
add_subdirectory( dav )
#add_subdirectory( ical )
add_subdirectory( imap )
add_subdirectory( kdeaccounts )
#if (Libkolab_FOUND AND Libkolabxml_FOUND)
#    add_subdirectory( kolabproxy )
#endif()

add_subdirectory( localbookmarks )

#add_subdirectory( maildir )

#add_subdirectory( openxchange )
add_subdirectory( pop3 )

#if( LibKGAPI2_FOUND )
#  add_subdirectory( google )
#endif()

#if( LibKFbAPI_FOUND )
#  add_subdirectory( facebook )
#endif()

add_subdirectory( shared )
add_subdirectory( birthdays )
#add_subdirectory( mixedmaildir )
add_subdirectory( mailtransport_dummy )
#add_subdirectory( mbox )
add_subdirectory( nntp )
add_subdirectory( vcarddir )
add_subdirectory( icaldir )
#add_subdirectory( vcard )
add_subdirectory( folderarchivesettings )
#add_subdirectory( kolab )
