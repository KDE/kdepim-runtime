project(kdepim-runtime)

# where to look first for cmake modules. This line must be the first one or cmake will use the system's FindFoo.cmake
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/modules)


############### search packages used by KDE ###############

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(KdepimLibs 4.5.60 REQUIRED)

find_package(Boost 1.34.0)
macro_log_feature(Boost_FOUND "Boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "1.34.0" "Required by Akonadi.")

find_package(Akonadi)
macro_log_feature(Akonadi_FOUND "Akonadi" "Akonadi server libraries" "http://pim.kde.org/akonadi" TRUE "" "Akonadi is required to build kdepim-runtime.")

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.30")
find_package(SharedMimeInfo)
macro_log_feature(SHARED_MIME_INFO_FOUND "SMI" "SharedMimeInfo" "http://freedesktop.org/wiki/Software/shared-mime-info" TRUE "0.30" "SharedMimeInfo is required.")

find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML applications" "http://xmlsoft.org" FALSE "" "Needed for building the knut Akonadi resource.")

set(SDO_MIN_VERSION 0.2)
set(SOPRANO_MIN_VERSION 2.3.70)

find_package(SharedDesktopOntologies ${SDO_MIN_VERSION})
macro_log_feature(SHAREDDESKTOPONTOLOGIES_FOUND "Shared desktop ontologies" "Desktop ontologies" "http://oscaf.sourceforge.net" TRUE "${SDO_MIN_VERSION}" "Ontologies necessary for the Nepomuk semantic desktop.")

find_package(Soprano)
macro_log_feature(Soprano_FOUND "Soprano" "Semantic Desktop Storing" "http://soprano.sourceforge.net" TRUE "${SOPRANO_MIN_VERSION}" "Soprano is needed for Nepomuk")

find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "The Nepomuk libraries" "http://www.kde.org" TRUE "" "Nepomuk extends the search and tagging functionalities in KMail and Akonadi")

find_package(Strigi)
macro_log_feature(STRIGI_FOUND "Strigi" "Index metadata of files" "http://strigi.sourceforge.net" FALSE "" "")

############### search programs & libraries used by kdepim-runtime ###############

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" TRUE "" "Required for building Akonadi.")

find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)
macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the message searchprovider")


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


############### Desktop vs. Mobile options ##############

option(KDEPIM_MOBILE_UI "Build UI for mobile devices instead of for desktops" FALSE)
if (KDEPIM_MOBILE_UI)
  macro_log_feature(QT_QTDECLARATIVE_FOUND "QtDeclarative" "Qt Declarative (QML) module" "http://qt.nokia.com" TRUE "4.7" "Required for building mobile UI")
  add_definitions( -DKDEPIM_MOBILE_UI )
endif (KDEPIM_MOBILE_UI)

############### Needed commands before building anything ###############

include_directories(${kdepim-runtime_SOURCE_DIR} ${kdepim-runtime_BINARY_DIR} ${AKONADI_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

if(WINCE)
  set(LIBRARY_TYPE STATIC)
  add_definitions(-DKDEPIM_STATIC_LIBS)
else(WINCE)
  set(LIBRARY_TYPE SHARED)
endif(WINCE)


############### Generate akonadi-config.h ###############

configure_file(akonadi-version.h.cmake "${kdepim-runtime_BINARY_DIR}/akonadi-version.h" @ONLY)


############### Macros ###############

# generates a D-Bus interface description from a KConfigXT file
macro(kcfg_generate_dbus_interface _kcfg _name)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
    ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
    ${_kcfg}
    > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
    ${_kcfg}
    )
endmacro(kcfg_generate_dbus_interface)

# resource tests
macro( akonadi_add_resourcetest _testname _script )
  if ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${EXECUTABLE_OUTPUT_PATH} )
  else ( ${EXECUTABLE_OUTPUT_PATH} )
    set( _exepath ${kdepim-runtime_BINARY_DIR}/resourcetester )
  endif ( ${EXECUTABLE_OUTPUT_PATH} )
  if (WIN32)
    set(_resourcetester ${_exepath}/resourcetester.bat)
  else (WIN32)
    set(_resourcetester ${_exepath}/resourcetester)
  endif (WIN32)
  if (UNIX)
    set(_resourcetester ${_resourcetester}.shell)
  endif (UNIX)

  add_test( akonadi-mysql-db-${_testname} akonaditest -c ${kdepim-runtime_SOURCE_DIR}/resourcetester/tests/unittestenv/config-mysql-db.xml ${_resourcetester} -c ${CMAKE_CURRENT_SOURCE_DIR}/${_script} )
endmacro( akonadi_add_resourcetest )

############### Now, we add the components ###############

add_subdirectory(resources) 
add_subdirectory(agents)
add_subdirectory(libkdepim-copy)
add_subdirectory(plugins)

#account wizzard needs to be ported to wince yet, does not work because of kross and kassistentdialoag
if (NOT WINCE)
add_subdirectory(accountwizard)
add_subdirectory(xml)
add_subdirectory(resourcetester)
add_subdirectory(kioslave)
add_subdirectory(kcm)
add_subdirectory(tray)
add_subdirectory(migration)
add_subdirectory(defaultsetup)
if (KDEPIMLIBS_KRESOURCES_LIBS)
  add_subdirectory(kresources)
endif (KDEPIMLIBS_KRESOURCES_LIBS)
endif(NOT WINCE)
if (QT_QTDECLARATIVE_FOUND)
  add_subdirectory(qml)
endif (QT_QTDECLARATIVE_FOUND)

#if(OPENSYNC_FOUND)
#  add_subdirectory(opensync)
#endif(OPENSYNC_FOUND)

############### Here we install some extra stuff  ###############

## install the MIME type spec file for KDEPIM specific MIME types
install(FILES kdepim-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

if(CMAKE_SOURCE_DIR STREQUAL "${kdepim-runtime_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${kdepim-runtime_SOURCE_DIR}")
