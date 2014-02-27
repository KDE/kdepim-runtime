project(kdepim-runtime)

# where to look first for cmake modules. This line must be the first one or cmake will use the system's FindFoo.cmake
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/modules)

add_definitions( -DDISABLE_NEPOMUK_LEGACY )

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
    set(KDEPIM_RUNTIME_DEV_VERSION "")
endif()

set(KDEPIM_RUNTIME_VERSION "4.12.3${KDEPIM_RUNTIME_DEV_VERSION}")

############### search-related options ################
option(KDEPIM_NO_NEPOMUK "Build without nepomuk dependency" FALSE)

if(KDEPIM_NO_NEPOMUK)
  add_definitions(-DKDEPIM_NO_NEPOMUK)
endif()	

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

############### search packages used by KDE ###############

# Kdelibs
find_package(KDE4 4.11.3 REQUIRED)
include(KDE4Defaults)

# KdepimLibs
find_package(KdepimLibs 4.12.3)
set_package_properties(KdepimLibs PROPERTIES DESCRIPTION "The KDEPIM libraries" URL "http://www.kde.org" TYPE REQUIRED)

#Boost
# Don't use BoostConfig.cmake
set(Boost_NO_BOOST_CMAKE TRUE)
find_package(Boost 1.34.0)
set_package_properties(Boost PROPERTIES DESCRIPTION "Boost C++ Libraries" URL "http://www.boost.org" TYPE REQUIRED PURPOSE "Boost is required by Akonadi")

# Akonadi
find_package(Akonadi 1.9.80 QUIET CONFIG)
set_package_properties(Akonadi PROPERTIES DESCRIPTION "Akonadi server libraries" URL "http://pim.kde.org/akonadi" TYPE REQUIRED PURPOSE "Access to PIM storage and services")

# shared-mime-info
find_package(SharedMimeInfo 0.30)
set_package_properties(SharedMimeInfo PROPERTIES DESCRIPTION "The shared-mime-info utility" URL "http://freedesktop.org/wiki/Software/shared-mime-info" TYPE REQUIRED PURPOSE "Information about filetypes")

if(NOT KDEPIM_NO_NEPOMUK)
  find_package(SharedDesktopOntologies 0.11)
  set_package_properties(SharedDesktopOntologies PROPERTIES DESCRIPTION "Shared Desktop Ontologies" URL "http://sourceforge.net/projects/oscaf/" TYPE REQUIRED PURPOSE "Ontologies necessary for the Nepomuk semantic desktop")

  find_package(Soprano 2.9)
  set_package_properties(Soprano PROPERTIES DESCRIPTION "C++/Qt4 framework for RDF data" URL "http://soprano.sourceforge.net" TYPE REQUIRED PURPOSE "Storage of semantic data")

  find_package(NepomukCore QUIET CONFIG)
  set_package_properties(NepomukCore PROPERTIES DESCRIPTION "Nepomuk core libraries"  URL "http://www.kde.org" TYPE REQUIRED PURPOSE "Nepomuk extends the search and tagging functionalities in KMail and Akonadi")
else()
  message(STATUS "Skipping detection of Nepomuk dependencies, feature is disabled.")
endif()

if(AKONADI_USE_STRIGI_SEARCH)
  find_package(Strigi)
  set_package_properties(Strigi PROPERTIES DESCRIPTION "Strigi" URL "https://projects.kde.org/strigi" TYPE REQUIRED)
endif()

# Libkolab
find_package(Libkolab 0.4 QUIET CONFIG)
set_package_properties(Libkolab PROPERTIES DESCRIPTION "libkolab" URL "http://mirror.kolabsys.com/pub/releases" TYPE OPTIONAL PURPOSE "The Kolab Format libraries are required to build the Kolab Groupware Resource")

# Libkolabxml
find_package(Libkolabxml QUIET CONFIG)
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

# Xsltproc
find_package(Xsltproc)
set_package_properties(Xsltproc PROPERTIES DESCRIPTION "XSLT processor from libxslt" TYPE REQUIRED PURPOSE "Required to generate D-Bus interfaces for all Akonadi resources.")

############### search programs & libraries used by kdepim-runtime ###############
if(Strigi_FOUND)
  find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
    PATHS
    /usr/lib
    /usr/local/lib
    ${LIB_INSTALL_DIR}
    $ENV{STRIGI_HOME}/lib
  )
  if(STRIGIQTDBUSCLIENT_LIBRARY)
    message(STATUS "Found the strigiqtdbusclient library.")
  endif()
  mark_as_advanced(STRIGIQTDBUSCLIENT_LIBRARY)
endif()

############### Load the CTest options ###############

# CMake is irritating and doesn't allow setting the tests timeout globally.
# Let's work around this. The global timeout is now 2 minutes.
set(_DartConfigFile "${CMAKE_BINARY_DIR}/DartConfiguration.tcl")
if(EXISTS ${_DartConfigFile})
  set(DartTestingTimeout "120")
  file(READ ${_DartConfigFile} _DartConfigFile_content)
  string(REGEX REPLACE "TimeOut: 1500" "TimeOut: ${DartTestingTimeout}" _DartConfigFile_content ${_DartConfigFile_content})
  file(WRITE ${_DartConfigFile} ${_DartConfigFile_content})
endif()

# CTestCustom.cmake has to be in the CTEST_BINARY_DIR.
# in the KDE build system, this is the same as CMAKE_BINARY_DIR.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)

option(KDEPIM_RUN_ISOLATED_TESTS "Run the isolated tests." FALSE)

############### Desktop vs. Mobile options ##############

option(KDEPIM_MOBILE_UI "Build UI for mobile devices instead of for desktops" FALSE)
if (KDEPIM_MOBILE_UI)
  add_definitions( -DKDEPIM_MOBILE_UI )
  if(NOT QT_QTDECLARATIVE_FOUND)
    message(FATAL_ERROR "The QtDeclarative library is required for building the mobile UI")
  endif()
endif ()

if(WINCE)
  set(LIBRARY_TYPE STATIC)
  add_definitions(-DKDEPIM_STATIC_LIBS)
else()
  set(LIBRARY_TYPE SHARED)
endif()

#FIXME: kde4_add_plugin doesn't have a parameter to build the plugins statically.
if (RUNTIME_PLUGINS_STATIC)
  set(SERIALIZER_TYPE STATIC)
endif ()

############### Enterprise build options #################

option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)

# config-enterprise.h is needed for both ENTERPRISE_BUILD and BUILD_EVERYTHING
configure_file(config-enterprise.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-enterprise.h )

############### Needed commands before building anything ###############

include_directories(${kdepim-runtime_SOURCE_DIR} ${kdepim-runtime_BINARY_DIR} ${AKONADI_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

############### Generate akonadi-config.h ###############

configure_file(akonadi-version.h.cmake "${kdepim-runtime_BINARY_DIR}/akonadi-version.h" @ONLY)


############### Macros ###############

# resource tests
macro( akonadi_add_resourcetest _testname _script )
  if ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${EXECUTABLE_OUTPUT_PATH} )
  else ()
    set( _exepath ${kdepim-runtime_BINARY_DIR}/resourcetester )
  endif ()
  if (WIN32)
    set(_resourcetester ${_exepath}/resourcetester.bat)
  else ()
    set(_resourcetester ${_exepath}/resourcetester)
  endif ()
  if (UNIX)
    set(_resourcetester ${_resourcetester}.shell)
  endif ()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_script} ${CMAKE_CURRENT_BINARY_DIR}/${_script} COPYONLY)
  if (KDEPIM_RUN_ISOLATED_TESTS)
    add_test( akonadi-mysql-db-${_testname} akonaditest -c ${kdepim-runtime_SOURCE_DIR}/resourcetester/tests/unittestenv/config-mysql-db.xml ${_resourcetester} -c ${CMAKE_CURRENT_BINARY_DIR}/${_script} )
  endif ()
endmacro( akonadi_add_resourcetest )

############### Now, we add the components ###############

add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(libkdepim-copy)
add_subdirectory(plugins)
add_subdirectory(accountwizard)
add_subdirectory(defaultsetup)

if (NOT WINCE)
add_subdirectory(resourcetester)
add_subdirectory(kioslave)
add_subdirectory(kcm)
add_subdirectory(tray)
add_subdirectory(migration)
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory(kresources)
endif ()
endif()
if (QT_QTDECLARATIVE_FOUND)
  add_subdirectory(qml)
endif ()

#if(OPENSYNC_FOUND)
#  add_subdirectory(opensync)
#endif(OPENSYNC_FOUND)

############### Here we install some extra stuff  ###############

## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

feature_summary(WHAT ALL
                     INCLUDE_QUIET_PACKAGES
                     FATAL_ON_MISSING_REQUIRED_PACKAGES
               )
