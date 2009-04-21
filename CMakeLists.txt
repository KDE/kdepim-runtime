project(KDEPIM_AKONADI)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
find_package(KdepimLibs REQUIRED)

macro_optional_find_package(Boost)
macro_log_feature(Boost_FOUND "boost" "Boost C++ Libraries" "http://www.boost.org" TRUE "1.33.1" "Required by Akonadi.")
if(NOT Boost_FOUND)
  macro_display_feature_log()
endif(NOT Boost_FOUND)

find_package(Akonadi REQUIRED)
include_directories(${AKONADI_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)
macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the mailthreaderagent and the message searchprovider")

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" TRUE "" "Required for building Akonadi.")
if(NOT XSLTPROC_EXECUTABLE)
  macro_display_feature_log()
endif(NOT XSLTPROC_EXECUTABLE)

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
#if(OPENSYNC_FOUND)
#  add_subdirectory(opensync)
#endif(OPENSYNC_FOUND)
add_subdirectory(akonadi_next)

if(Nepomuk_FOUND)
  add_subdirectory(nepomuk)
endif(Nepomuk_FOUND)

#### MIME type spezializations

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo REQUIRED)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
