find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)

macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the mailthreaderagent and the message searchprovider")

if (XSLTPROC_EXECUTABLE)
  # generates a D-Bus interface description from a KConfigXT file
  macro( kcfg_generate_dbus_interface _kcfg _name )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
              ${CMAKE_INSTALL_PREFIX}/share/apps/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
              > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${CMAKE_INSTALL_PREFIX}/share/apps/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
    )
  endmacro( kcfg_generate_dbus_interface )
  add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

add_subdirectory(clients)
add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(kioslave)
add_subdirectory(plugins)
add_subdirectory(kabc)
add_subdirectory(kcal)
add_subdirectory(kcm)
add_subdirectory(tray)

if( Nepomuk_FOUND )
  add_subdirectory(nepomuk)
endif( Nepomuk_FOUND )

#### MIME type spezializations

set(SHARED_MIME_INFO_MINIMUM_VERSION "0.20")
find_package(SharedMimeInfo REQUIRED)

