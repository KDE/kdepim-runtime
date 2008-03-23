if (AKONADI_FOUND)
configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)

INCLUDE(CheckIncludeFiles)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)

find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)

macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the mailthreaderagent and the message searchprovider")

if (XSLTPROC_EXECUTABLE)
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

########### install files ###############
install(FILES akonadi-mime.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})

endif (AKONADI_FOUND)
