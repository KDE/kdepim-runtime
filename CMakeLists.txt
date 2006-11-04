configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h )

if (XSLTPROC_EXECUTABLE)
add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

add_subdirectory(libakonadi)
add_subdirectory(clients)
add_subdirectory(resources)
add_subdirectory(searchproviders)
add_subdirectory(kioslave)
