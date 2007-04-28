find_package( Boost REQUIRED )
macro_log_feature(Boost_FOUND "boost" "Boost C++ Libraries" "http://www.boost.org/" TRUE "" "Required by Akonadi")

configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)

if (XSLTPROC_EXECUTABLE)
add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

add_subdirectory(libakonadi)
add_subdirectory(clients)
add_subdirectory(resources)
add_subdirectory(searchproviders)
add_subdirectory(kioslave)
add_subdirectory(plugins)
add_subdirectory(models)
