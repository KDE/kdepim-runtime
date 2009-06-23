project(kdepim-runtime)

# where to look first for cmake modules. This line must be the first one or cmake will use the system's FindFoo.cmake
set(CMAKE_MODULE_PATH ${kdepim-runtime_SOURCE_DIR}/cmake/modules)

############### search packages used by KDE ###############

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(KdepimLibs 4.2.87 REQUIRED)

find_package(Boost 1.33.1)
macro_log_feature(Boost_FOUND "Boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "1.33.1" "Required by Akonadi.")

find_package(Akonadi)
macro_log_feature(Akonadi_FOUND "Akonadi" "Akonadi server libraries (from kdesupport)" "http://pim.kde.org/akonadi" TRUE "" "Akonadi is required to build kdepim-runtime.")

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo)
macro_log_feature(SHARED_MIME_INFO_FOUND "SMI" "SharedMimeInfo" "http://freedesktop.org/wiki/Software/shared-mime-info" TRUE "0.20" "SharedMimeInfo is required.")

find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML applications" "http://xmlsoft.org" FALSE "" "Needed for building the knut Akonadi resource.")

find_package(Nepomuk)
macro_log_feature(Nepomuk_FOUND "Nepomuk" "The Nepomuk libraries" "http://www.kde.org" FALSE "" "Nepomuk extends the search and tagging functionalities in KMail and Akonadi")

find_package(LibKNotificationItem-1_New)
macro_log_feature(LIBKNOTIFICATIONITEM-1_FOUND "LibKNotificationItem-1" "An experimental library" "svn://anonsvn.kde.org/home/kde/trunk/KDE/kdelibs/experimental" TRUE "" "LibKNotificationItem-1 is an unfinished library which is required by korgac and akonaditray.")


############### search programs & libraries used by kpim-runtime ###############

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


############### Needed commands before building anything ###############

include_directories(${kdepim-runtime_SOURCE_DIR} ${kdepim-runtime_BINARY_DIR} ${AKONADI_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${KDE4_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")


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


############### Now, we add the components ###############

add_subdirectory(libkdepim-copy)
add_subdirectory(xml)
add_subdirectory(clients)
add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(kioslave)
add_subdirectory(plugins)
add_subdirectory(kabc)
add_subdirectory(kcal)
add_subdirectory(kcm)
add_subdirectory(tray)
add_subdirectory(migration)
add_subdirectory(defaultsetup)
add_subdirectory(akonadi_next)
add_subdirectory(kresources)
add_subdirectory(akonadiconsole)

#if(OPENSYNC_FOUND)
#  add_subdirectory(opensync)
#endif(OPENSYNC_FOUND)


if(CMAKE_SOURCE_DIR STREQUAL "${kdepim-runtime_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${kdepim-runtime_SOURCE_DIR}")
